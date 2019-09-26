#include "study_coroutine_channel.h"

using study::coroutine::Channel;

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_channel_construct, 0, 0, 0)
    ZEND_ARG_INFO(0, capacity)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_channel_push, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_channel_pop, 0, 0, 0)
    ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

/**
 * Define zend class entry
 */
zend_class_entry study_coro_channel_ce;
zend_class_entry *study_coro_channel_ce_ptr;

static zend_object_handlers study_coro_channel_handlers;

typedef struct
{
    Channel *chan;
    zend_object std;
} coro_chan;

static coro_chan* study_coro_channel_fetch_object(zend_object *obj)
{
    return (coro_chan *)((char *)obj - study_coro_channel_handlers.offset);
}

static zend_object* study_coro_channel_create_object(zend_class_entry *ce)
{
    coro_chan *chan_t = (coro_chan *)ecalloc(1, sizeof(coro_chan) + zend_object_properties_size(ce));
    zend_object_std_init(&chan_t->std, ce);
    object_properties_init(&chan_t->std, ce);
    chan_t->std.handlers = &study_coro_channel_handlers;
    return &chan_t->std;
}

static void study_coro_channel_free_object(zend_object *object)
{
    coro_chan *chan_t = (coro_chan *)study_coro_channel_fetch_object(object);
    Channel *chan = chan_t->chan;
    if (chan)
    {
        while (!chan->empty())
        {
            zval *data;
            data = (zval *)chan->pop_data();
            zval_ptr_dtor(data);
            efree(data);
        }

        delete chan;
    }
    zend_object_std_dtor(&chan_t->std);
}

static PHP_METHOD(study_coro_channel, __construct)
{
    coro_chan *chan_t;
    zend_long capacity = 1;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(capacity)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if (capacity <= 0)
    {
        capacity = 1;
    }

    chan_t = (coro_chan *)study_coro_channel_fetch_object(Z_OBJ_P(getThis()));
    chan_t->chan = new Channel(capacity);

    zend_update_property_long(study_coro_channel_ce_ptr, getThis(), ZEND_STRL("capacity"), capacity);
}

static PHP_METHOD(study_coro_channel, push)
{
    coro_chan *chan_t;
    Channel *chan;
    zval *zdata;
    double timeout = -1;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(zdata)
        Z_PARAM_OPTIONAL
        Z_PARAM_DOUBLE(timeout)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    chan_t = (coro_chan *)study_coro_channel_fetch_object(Z_OBJ_P(getThis()));
    chan = chan_t->chan;

    Z_TRY_ADDREF_P(zdata);
    zdata = st_zval_dup(zdata);

    if (!chan->push(zdata, timeout))
    {
        Z_TRY_DELREF_P(zdata);
        efree(zdata);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

static PHP_METHOD(study_coro_channel, pop)
{
    coro_chan *chan_t;
    Channel *chan;
    double timeout = -1;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_DOUBLE(timeout)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    chan_t = (coro_chan *)study_coro_channel_fetch_object(Z_OBJ_P(getThis()));
    chan = chan_t->chan;
    zval *zdata = (zval *)chan->pop(timeout);
    if (!zdata)
    {
        RETURN_FALSE;
    }
    RETVAL_ZVAL(zdata, 0, 0);
    efree(zdata);
}

static const zend_function_entry study_coro_channel_methods[] =
{
    PHP_ME(study_coro_channel, __construct, arginfo_study_coro_channel_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR) // ZEND_ACC_CTOR is used to declare that this method is a constructor of this class.
    PHP_ME(study_coro_channel, push, arginfo_study_coro_channel_push, ZEND_ACC_PUBLIC)
    PHP_ME(study_coro_channel, pop, arginfo_study_coro_channel_pop, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

void study_coro_channel_init()
{
    INIT_NS_CLASS_ENTRY(study_coro_channel_ce, "Study", "Coroutine\\Channel", study_coro_channel_methods);
    study_coro_channel_ce_ptr = zend_register_internal_class(&study_coro_channel_ce TSRMLS_CC); // Registered in the Zend Engine
    memcpy(&study_coro_channel_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    ST_SET_CLASS_CUSTOM_OBJECT(study_coro_channel, study_coro_channel_create_object, study_coro_channel_free_object, coro_chan, std);

    zend_declare_property_long(study_coro_channel_ce_ptr, ZEND_STRL("capacity"), 1, ZEND_ACC_PUBLIC);
}