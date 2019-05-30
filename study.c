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
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_study.h"
#include <pthread.h>

static void (*orig_interrupt_function)(zend_execute_data *execute_data);

void schedule();
static void create_scheduler_thread();
static void new_interrupt_function(zend_execute_data *execute_data);

void init()
{
	orig_interrupt_function = zend_interrupt_function;
	zend_interrupt_function = new_interrupt_function;
}

static void new_interrupt_function(zend_execute_data *execute_data)
{
	php_printf("yeild coroutine\n");
	if (orig_interrupt_function)
    {
        orig_interrupt_function(execute_data);
    }
}

void schedule()
{
    while (1)
    {
        EG(vm_interrupt) = 1;
        usleep(5000);
    }
}

static void create_scheduler_thread()
{
    pthread_t pidt;

    if (pthread_create(&pidt, NULL, (void * (*)(void *)) schedule, NULL) < 0)
    {
        php_printf("pthread_create[PHPCoroutine Scheduler] failed");
    }
}

PHP_FUNCTION(start_interrupt) {
	init();
	create_scheduler_thread();
};

PHP_FUNCTION(test) {
    zend_string *str;

    str = zend_string_init("foo", strlen("foo"), 0);
    php_printf("This is my string: %s\n", ZSTR_VAL(str));
    php_printf("It is %zd char long\n", ZSTR_LEN(str));

    zend_string_release(str);
};

PHP_MINIT_FUNCTION(study)
{
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(study)
{
	return SUCCESS;
}

PHP_RINIT_FUNCTION(study)
{
#if defined(COMPILE_DL_STUDY) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
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
	PHP_FE(start_interrupt,	NULL)
	PHP_FE(test, NULL)
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
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(study)
#endif
