#include "study_coroutine_socket.h"

using study::coroutine::Socket;

typedef struct
{
    Socket *socket;
    zend_object std;
} coro_socket;

#define ST_BAD_SOCKET ((Socket *)-1)

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_socket_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_socket_construct, 0, 0, 2)
    ZEND_ARG_INFO(0, domain)
    ZEND_ARG_INFO(0, type)
    ZEND_ARG_INFO(0, protocol)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_socket_bind, 0, 0, 2)
    ZEND_ARG_INFO(0, host)
    ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_socket_listen, 0, 0, 0)
    ZEND_ARG_INFO(0, backlog)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_socket_recv, 0, 0, 0)
    ZEND_ARG_INFO(0, length)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_socket_send, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

/**
 * Define zend class entry
 */
zend_class_entry study_coro_socket_ce;
zend_class_entry *study_coro_socket_ce_ptr;

static zend_object_handlers study_coro_socket_handlers;

static coro_socket* study_coro_socket_fetch_object(zend_object *obj)
{
    return (coro_socket *) ((char *) obj - study_coro_socket_handlers.offset);
}

static zend_object* study_coro_socket_create_object(zend_class_entry *ce)
{
    coro_socket *sock = (coro_socket *) ecalloc(1, sizeof(coro_socket) + zend_object_properties_size(ce));
    zend_object_std_init(&sock->std, ce);
    object_properties_init(&sock->std, ce);
    sock->std.handlers = &study_coro_socket_handlers;
    return &sock->std;
}

static void study_coro_socket_free_object(zend_object *object)
{
    coro_socket *sock = (coro_socket *) study_coro_socket_fetch_object(object);
    if (sock->socket && sock->socket != ST_BAD_SOCKET)
    {
        sock->socket->close();
        delete sock->socket;
    }
    zend_object_std_dtor(&sock->std);
}

static PHP_METHOD(study_coro_socket, __construct)
{
    zend_long domain;
    zend_long type;
    zend_long protocol;
    coro_socket *socket_t;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_LONG(domain)
        Z_PARAM_LONG(type)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(protocol)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    socket_t->socket = new Socket(domain, type, protocol);

    zend_update_property_long(study_coro_socket_ce_ptr, getThis(), ZEND_STRL("fd"), socket_t->socket->get_fd());
}

static PHP_METHOD(study_coro_socket, bind)
{
    zend_long port;
    Socket *socket;
    coro_socket *socket_t;
    zval *zhost;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(zhost)
        Z_PARAM_LONG(port)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    socket = socket_t->socket;

    if (socket->bind(ST_SOCK_TCP, Z_STRVAL_P(zhost), port) < 0)
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

static PHP_METHOD(study_coro_socket, listen)
{
    zend_long backlog = 512;
    Socket *socket;
    coro_socket *socket_t;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(backlog)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    socket = socket_t->socket;

    if (socket->listen(backlog) < 0)
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

static PHP_METHOD(study_coro_socket, accept)
{
    coro_socket *server_socket_t;
    coro_socket *conn_socket_t;
    Socket *server_socket;
    Socket *conn_socket;

    server_socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    server_socket = server_socket_t->socket;
    conn_socket = server_socket->accept();
    if (!conn_socket)
    {
        RETURN_NULL();
    }
    zend_object *conn = study_coro_socket_create_object(study_coro_socket_ce_ptr);
    conn_socket_t = (coro_socket *)study_coro_socket_fetch_object(conn);
    conn_socket_t->socket = conn_socket;
    ZVAL_OBJ(return_value, &(conn_socket_t->std));
    zend_update_property_long(study_coro_socket_ce_ptr, return_value, ZEND_STRL("fd"), conn_socket_t->socket->get_fd());
}

static PHP_METHOD(study_coro_socket, recv)
{
    ssize_t ret;
    zend_long length = 65536;
    coro_socket *conn_socket_t;
    Socket *conn_socket;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(length)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    Socket::init_read_buffer();

    conn_socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    conn_socket = conn_socket_t->socket;
    ret = conn_socket->recv(Socket::read_buffer, Socket::read_buffer_len);
    if (ret == 0)
    {
        zend_update_property_long(study_coro_socket_ce_ptr, getThis(), ZEND_STRL("errCode"), ST_ERROR_SESSION_CLOSED_BY_CLIENT);
        zend_update_property_string(study_coro_socket_ce_ptr, getThis(), ZEND_STRL("errMsg"), st_strerror(ST_ERROR_SESSION_CLOSED_BY_CLIENT));
        RETURN_FALSE;
    }
    if (ret < 0)
    {
        php_error_docref(NULL, E_WARNING, "recv error");
        RETURN_FALSE;
    }
    Socket::read_buffer[ret] = '\0';
    RETURN_STRING(Socket::read_buffer);
}

static PHP_METHOD(study_coro_socket, send)
{
    ssize_t ret;
    char *data;
    size_t length;
    coro_socket *conn_socket_t;
    Socket *conn_socket;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(data, length)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    conn_socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    conn_socket = conn_socket_t->socket;
    ret = conn_socket->send(data, length);
    if (ret < 0)
    {
        php_error_docref(NULL, E_WARNING, "send error");
        RETURN_FALSE;
    }
    RETURN_LONG(ret);
}

static PHP_METHOD(study_coro_socket, close)
{
    int ret;
    coro_socket *conn_socket_t;

    conn_socket_t = (coro_socket *)study_coro_socket_fetch_object(Z_OBJ_P(getThis()));
    ret = conn_socket_t->socket->close();
    if (ret < 0)
    {
        php_error_docref(NULL, E_WARNING, "close error");
        RETURN_FALSE;
    }
    delete conn_socket_t->socket;
    conn_socket_t->socket = ST_BAD_SOCKET;

    RETURN_TRUE;
}

static const zend_function_entry study_coro_socket_methods[] =
{
    PHP_ME(study_coro_socket, __construct, arginfo_study_coro_socket_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(study_coro_socket, bind, arginfo_study_coro_socket_bind, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_socket, listen, arginfo_study_coro_socket_listen, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_socket, accept, arginfo_study_coro_socket_void, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_socket, recv, arginfo_study_coro_socket_recv, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_socket, send, arginfo_study_coro_socket_send, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_socket, close, arginfo_study_coro_socket_void, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

void study_coro_socket_init(int module_number)
{
    INIT_NS_CLASS_ENTRY(study_coro_socket_ce, "Study", "Coroutine\\Socket", study_coro_socket_methods);
    memcpy(&study_coro_socket_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    study_coro_socket_ce_ptr = zend_register_internal_class(&study_coro_socket_ce TSRMLS_CC); // Registered in the Zend Engine
    ST_SET_CLASS_CUSTOM_OBJECT(study_coro_socket, study_coro_socket_create_object, study_coro_socket_free_object, coro_socket, std);

    zend_declare_property_long(study_coro_socket_ce_ptr, ZEND_STRL("fd"), -1, ZEND_ACC_PUBLIC);
    zend_declare_property_long(study_coro_socket_ce_ptr, ZEND_STRL("errCode"), 0, ZEND_ACC_PUBLIC);
    zend_declare_property_string(study_coro_socket_ce_ptr, ZEND_STRL("errMsg"), "", ZEND_ACC_PUBLIC);

    if (!zend_hash_str_find_ptr(&module_registry, ZEND_STRL("sockets")))
    {
        REGISTER_LONG_CONSTANT("AF_INET", AF_INET, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("SOCK_STREAM", SOCK_STREAM, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("IPPROTO_IP", IPPROTO_IP, CONST_CS | CONST_PERSISTENT);
    }
}