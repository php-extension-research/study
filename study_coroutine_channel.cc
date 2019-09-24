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

static PHP_METHOD(study_coro_channel, __construct)
{
    zval zchan;
    zend_long capacity = 1;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(capacity)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if (capacity <= 0)
    {
        capacity = 1;
    }

    zend_update_property_long(study_coro_channel_ce_ptr, getThis(), ZEND_STRL("capacity"), capacity);

    Channel *chan = new Channel(capacity);
    ZVAL_PTR(&zchan, chan);
    zend_update_property(study_coro_channel_ce_ptr, getThis(), ZEND_STRL("zchan"), &zchan);
}

static PHP_METHOD(study_coro_channel, push)
{
    zval *zchan;
    Channel *chan;
    zval *zdata;
    double timeout = -1;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(zdata)
        Z_PARAM_OPTIONAL
        Z_PARAM_DOUBLE(timeout)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    zchan = st_zend_read_property(study_coro_channel_ce_ptr, getThis(), ZEND_STRL("zchan"), 0);
    chan = (Channel *)Z_PTR_P(zchan);

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
    zval *zchan;
    Channel *chan;
    double timeout = -1;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_DOUBLE(timeout)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    zchan = st_zend_read_property(study_coro_channel_ce_ptr, getThis(), ZEND_STRL("zchan"), 0);
    chan = (Channel *)Z_PTR_P(zchan);
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
    zval zchan;

    INIT_NS_CLASS_ENTRY(study_coro_channel_ce, "Study", "Coroutine\\Channel", study_coro_channel_methods);
    study_coro_channel_ce_ptr = zend_register_internal_class(&study_coro_channel_ce TSRMLS_CC); // Registered in the Zend Engine

    zend_declare_property(study_coro_channel_ce_ptr, ZEND_STRL("zchan"), &zchan, ZEND_ACC_PUBLIC);
    zend_declare_property_long(study_coro_channel_ce_ptr, ZEND_STRL("capacity"), 1, ZEND_ACC_PUBLIC);
}