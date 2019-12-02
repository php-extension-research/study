#include "study_runtime.h"
#include "study_coroutine_socket.h"

using study::coroutine::Socket;

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_runtime_void, 0, 0, 0)
ZEND_END_ARG_INFO()

extern PHP_METHOD(study_coroutine_util, sleep);
static void hook_func(const char *name, size_t name_len, zif_handler handler);
static int socket_close(php_stream *stream, int close_handle);
static int socket_set_option(php_stream *stream, int option, int value, void *ptrparam);

struct php_study_netstream_data_t
{
    php_netstream_data_t stream;
    Socket *socket;
};

static php_stream_ops socket_ops
{
    NULL,
    NULL,
    socket_close,
    NULL,
    "tcp_socket/coroutine",
    NULL, /* seek */
    NULL,
    NULL,
    socket_set_option,
};

enum
{
    STREAM_XPORT_OP_BIND,
    STREAM_XPORT_OP_CONNECT,
    STREAM_XPORT_OP_LISTEN,
    STREAM_XPORT_OP_ACCEPT,
    STREAM_XPORT_OP_CONNECT_ASYNC,
    STREAM_XPORT_OP_GET_NAME,
    STREAM_XPORT_OP_GET_PEER_NAME,
    STREAM_XPORT_OP_RECV,
    STREAM_XPORT_OP_SEND,
    STREAM_XPORT_OP_SHUTDOWN,
};

php_stream *socket_create(const char *proto, size_t protolen,
    const char *resourcename, size_t resourcenamelen,
    const char *persistent_id, int options, int flags,
    struct timeval *timeout,
    php_stream_context *context STREAMS_DC
)
{
    php_stream *stream;
    php_study_netstream_data_t *abstract;
    Socket *sock;

    sock = new Socket(AF_INET, SOCK_STREAM, 0);
    abstract = (php_study_netstream_data_t*) ecalloc(1, sizeof(*abstract));
    abstract->socket = sock;
    abstract->stream.socket = sock->get_fd();

    if (timeout)
    {
        abstract->stream.timeout = *timeout;
    }
    else
    {
        abstract->stream.timeout.tv_sec = -1;
    }

    persistent_id = nullptr;
    stream = php_stream_alloc_rel(&socket_ops, abstract, persistent_id, "r+");
    if (stream == NULL)
    {
        delete sock;
    }
    return stream;
}

static int socket_close(php_stream *stream, int close_handle)
{
    php_study_netstream_data_t *abstract = (php_study_netstream_data_t *) stream->abstract;
    Socket *sock = (Socket*) abstract->socket;

    sock->close();
    delete sock;
    efree(abstract);
    return 0;
}

/**
 * copy from php src file: xp_socket.c
 */
static inline char *parse_ip_address_ex(const char *str, size_t str_len, int *portno, int get_err, zend_string **err)
{
    char *colon;
    char *host = NULL;

#ifdef HAVE_IPV6
    char *p;

    if (*(str) == '[' && str_len > 1)
    {
        /* IPV6 notation to specify raw address with port (i.e. [fe80::1]:80) */
        p = (char *)memchr(str + 1, ']', str_len - 2);
        if (!p || *(p + 1) != ':')
        {
            if (get_err)
            {
                *err = strpprintf(0, "Failed to parse IPv6 address \"%s\"", str);
            }
            return NULL;
        }
        *portno = atoi(p + 2);
        return estrndup(str + 1, p - str - 1);
    }
#endif
    if (str_len)
    {
        colon = (char *)memchr(str, ':', str_len - 1);
    }
    else
    {
        colon = NULL;
    }
    if (colon)
    {
        *portno = atoi(colon + 1);
        host = estrndup(str, colon - str);
    }
    else
    {
        if (get_err)
        {
            *err = strpprintf(0, "Failed to parse address \"%s\"", str);
        }
        return NULL;
    }

    return host;
}

/**
 * copy from php src file: xp_socket.c
 */
static inline char *parse_ip_address(php_stream_xport_param *xparam, int *portno)
{
    return parse_ip_address_ex(xparam->inputs.name, xparam->inputs.namelen, portno, xparam->want_errortext, &xparam->outputs.error_text);
}

static int php_study_tcp_sockop_bind(php_stream *stream, php_study_netstream_data_t *abstract, php_stream_xport_param *xparam)
{
    char *host = NULL;
    int portno;

    host = parse_ip_address(xparam, &portno);

    if (host == NULL)
    {
        return -1;
    }

    int ret = abstract->socket->bind(ST_SOCK_TCP, host, portno);

    if (host)
    {
        efree(host);
    }
    return ret;
}

static int socket_set_option(php_stream *stream, int option, int value, void *ptrparam)
{
    php_study_netstream_data_t *abstract = (php_study_netstream_data_t *) stream->abstract;
    php_stream_xport_param *xparam;

    switch(option)
    {
    case PHP_STREAM_OPTION_XPORT_API:
        xparam = (php_stream_xport_param *)ptrparam;

        switch(xparam->op)
        {
        case STREAM_XPORT_OP_BIND:
            xparam->outputs.returncode = php_study_tcp_sockop_bind(stream, abstract, xparam);
            return PHP_STREAM_OPTION_RETURN_OK;
        case STREAM_XPORT_OP_LISTEN:
            xparam->outputs.returncode = abstract->socket->listen(xparam->inputs.backlog);
            return PHP_STREAM_OPTION_RETURN_OK;
        default:
            /* fall through */
            ;
        }
    }
    return PHP_STREAM_OPTION_RETURN_OK;
}

static PHP_METHOD(study_runtime, enableCoroutine)
{
    hook_func(ZEND_STRL("sleep"), zim_study_coroutine_util_sleep);
    php_stream_xport_register("tcp", socket_create);
}

static const zend_function_entry study_runtime_methods[] =
{
    PHP_ME(study_runtime, enableCoroutine, arginfo_study_runtime_void, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_FE_END
};

static void hook_func(const char *name, size_t name_len, zif_handler new_handler)
{
    zend_function *ori_f = (zend_function *) zend_hash_str_find_ptr(EG(function_table), name, name_len);
    ori_f->internal_function.handler = new_handler;
}

/**
 * Define zend class entry
 */
zend_class_entry study_runtime_ce;
zend_class_entry *study_runtime_ce_ptr;

void study_runtime_init()
{
    INIT_NS_CLASS_ENTRY(study_runtime_ce, "Study", "Runtime", study_runtime_methods);
    study_runtime_ce_ptr = zend_register_internal_class(&study_runtime_ce TSRMLS_CC); // Registered in the Zend Engine
}