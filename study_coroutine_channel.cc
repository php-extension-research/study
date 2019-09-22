#include "study_coroutine_channel.h"

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coro_channel_construct, 0, 0, 0)
    ZEND_ARG_INFO(0, capacity)
ZEND_END_ARG_INFO()

/**
 * Define zend class entry
 */
zend_class_entry study_coro_channel_ce;
zend_class_entry *study_coro_channel_ce_ptr;

static PHP_METHOD(study_coro_channel, __construct)
{
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
}

static const zend_function_entry study_coro_channel_methods[] =
{
    PHP_ME(study_coro_channel, __construct, arginfo_study_coro_channel_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR) // ZEND_ACC_CTOR is used to declare that this method is a constructor of this class.
    PHP_FE_END
};

void study_coro_channel_init()
{
    INIT_NS_CLASS_ENTRY(study_coro_channel_ce, "Study", "Coroutine\\Channel", study_coro_channel_methods);
    study_coro_channel_ce_ptr = zend_register_internal_class(&study_coro_channel_ce TSRMLS_CC); // Registered in the Zend Engine

    zend_declare_property_long(study_coro_channel_ce_ptr, ZEND_STRL("capacity"), 1, ZEND_ACC_PUBLIC);
}