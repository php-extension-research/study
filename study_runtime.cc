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

static int socket_set_option(php_stream *stream, int option, int value, void *ptrparam)
{
    return 0;
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