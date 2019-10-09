/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: codinghuang                                                             |
  +----------------------------------------------------------------------+
*/

#include "php_study.h"

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_study_coroutine_create, 0, 0, 1)
    ZEND_ARG_CALLABLE_INFO(0, func, 0)
ZEND_END_ARG_INFO()

PHP_FUNCTION(study_coroutine_create);

PHP_FUNCTION(study_event_init)
{
    int ret;
    ret = st_event_init();
    if (ret < 0)
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

PHP_FUNCTION(study_event_wait)
{
    int ret;
    ret = st_event_wait();
    if (ret < 0)
    {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

PHP_MINIT_FUNCTION(study)
{
    study_coroutine_util_init();
    study_coro_server_init(module_number);
    study_coro_channel_init();
    study_coro_socket_init(module_number);
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(study)
{
    return SUCCESS;
}

PHP_RINIT_FUNCTION(study)
{
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(study)
{
    return SUCCESS;
}

PHP_MINFO_FUNCTION(study)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "study support", "enabled");
    php_info_print_table_end();
}

const zend_function_entry study_functions[] = {
    PHP_FE(study_coroutine_create, arginfo_study_coroutine_create)
    PHP_FALIAS(sgo, study_coroutine_create, arginfo_study_coroutine_create)
    PHP_FE(study_event_init, arginfo_study_void)
    PHP_FE(study_event_wait, arginfo_study_void)
    PHP_FE_END
};

zend_module_entry study_module_entry = {
    STANDARD_MODULE_HEADER,
    "study",
    study_functions,
    PHP_MINIT(study),
    PHP_MSHUTDOWN(study),
    PHP_RINIT(study),
    PHP_RSHUTDOWN(study),
    PHP_MINFO(study),
    PHP_STUDY_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_STUDY
ZEND_GET_MODULE(study)
#endif
