#include "study_coroutine_server.h"
#include "study_coroutine_socket.h"
#include "log.h"

using study::phpcoroutine::Server;
using study::PHPCoroutine;
using study::coroutine::Socket;

Server::Server(char *host, int port)
{
    socket = new Socket(AF_INET, SOCK_STREAM, 0);
    if (socket->bind(ST_SOCK_TCP, host, port) < 0)
    {
        stWarn("Error has occurred: (errno %d) %s", errno, strerror(errno));
        return;
    }
    if (socket->listen(512) < 0)
    {
        return;
    }
}

Server::~Server()
{
}

bool Server::start()
{
    zval zsocket;
    running = true;

    while (running)
    {
        Socket* conn = socket->accept();
        if (!conn)
        {
            return false;
        }

        php_study_init_socket_object(&zsocket, conn);
        PHPCoroutine::create(&(handler->fcc), 1, &zsocket);
        zval_dtor(&zsocket);
    }
    return true;
}

bool Server::shutdown()
{
    running = false;
    return true;
}

void Server::set_handler(php_study_fci_fcc *_handler)
{
    handler = _handler;
}

php_study_fci_fcc* Server::get_handler()
{
    return handler;
}

/**
 * Define zend class entry
 */
zend_class_entry study_coro_server_ce;
zend_class_entry *study_coro_server_ce_ptr;

static zend_object_handlers study_coro_server_handlers;

typedef struct
{
    Server *serv;
    zend_object std;
} coro_serv;

static coro_serv* study_coro_server_fetch_object(zend_object *obj)
{
    return (coro_serv *)((char *)obj - study_coro_server_handlers.offset);
}

static zend_object* study_coro_server_create_object(zend_class_entry *ce)
{
    coro_serv *serv_t = (coro_serv *)ecalloc(1, sizeof(coro_serv) + zend_object_properties_size(ce));
    zend_object_std_init(&serv_t->std, ce);
    object_properties_init(&serv_t->std, ce);
    serv_t->std.handlers = &study_coro_server_handlers;
    return &serv_t->std;
}

static void study_coro_server_free_object(zend_object *object)
{
    coro_serv *serv_t = (coro_serv *) study_coro_server_fetch_object(object);
    if (serv_t->serv)
    {
        php_study_fci_fcc *handler = serv_t->serv->get_handler();
        if (handler)
        {
            efree(handler);
            serv_t->serv->set_handler(nullptr);
        }
        delete serv_t->serv;
    }
    zend_object_std_dtor(&serv_t->std);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_server_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_server_construct, 0, 0, 2)
    ZEND_ARG_INFO(0, host)
    ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_server_ser_handler, 0, 0, 1)
    ZEND_ARG_CALLABLE_INFO(0, func, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(study_coro_server, __construct)
{
    zend_long port;
    coro_serv *server_t;
    zval *zhost;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(zhost)
        Z_PARAM_LONG(port)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    server_t = (coro_serv *)study_coro_server_fetch_object(Z_OBJ_P(getThis()));
    server_t->serv = new Server(Z_STRVAL_P(zhost), port);

    zend_update_property_string(study_coro_server_ce_ptr, getThis(), ZEND_STRL("host"), Z_STRVAL_P(zhost));
    zend_update_property_long(study_coro_server_ce_ptr, getThis(), ZEND_STRL("port"), port);
}

PHP_METHOD(study_coro_server, start)
{
    coro_serv *server_t;

    server_t = (coro_serv *)study_coro_server_fetch_object(Z_OBJ_P(getThis()));
    if (server_t->serv->start() == false)
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

PHP_METHOD(study_coro_server, shutdown)
{
    coro_serv *server_t;

    server_t = (coro_serv *)study_coro_server_fetch_object(Z_OBJ_P(getThis()));
    if (server_t->serv->shutdown() == false)
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

PHP_METHOD(study_coro_server, set_handler)
{
    coro_serv *server_t;
    php_study_fci_fcc *handle_fci_fcc;

    handle_fci_fcc = (php_study_fci_fcc *)emalloc(sizeof(php_study_fci_fcc));

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_FUNC(handle_fci_fcc->fci, handle_fci_fcc->fcc)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    server_t = (coro_serv *)study_coro_server_fetch_object(Z_OBJ_P(getThis()));
    server_t->serv->set_handler(handle_fci_fcc);
}

static const zend_function_entry study_coroutine_server_coro_methods[] =
{
    PHP_ME(study_coro_server, __construct, arginfo_study_coro_server_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR) // ZEND_ACC_CTOR is used to declare that this method is a constructor of this class.
    PHP_ME(study_coro_server, start, arginfo_study_coro_server_void, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_server, shutdown, arginfo_study_coro_server_void, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_server, set_handler, arginfo_study_coro_server_ser_handler, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

void study_coro_server_init(int module_number)
{
    INIT_NS_CLASS_ENTRY(study_coro_server_ce, "Study", "Coroutine\\Server", study_coroutine_server_coro_methods);

    memcpy(&study_coro_server_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    study_coro_server_ce_ptr = zend_register_internal_class(&study_coro_server_ce TSRMLS_CC); // Registered in the Zend Engine
    ST_SET_CLASS_CUSTOM_OBJECT(study_coro_server, study_coro_server_create_object, study_coro_server_free_object, coro_serv, std);

    zend_declare_property_string(study_coro_server_ce_ptr, ZEND_STRL("host"), "", ZEND_ACC_PUBLIC);
    zend_declare_property_long(study_coro_server_ce_ptr, ZEND_STRL("port"), -1, ZEND_ACC_PUBLIC);
    zend_declare_property_long(study_coro_server_ce_ptr, ZEND_STRL("errCode"), 0, ZEND_ACC_PUBLIC);
    zend_declare_property_string(study_coro_server_ce_ptr, ZEND_STRL("errMsg"), "", ZEND_ACC_PUBLIC);
}
