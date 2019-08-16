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

#ifndef PHP_STUDY_H
#define PHP_STUDY_H

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"

#include "study.h"

#define PHP_STUDY_VERSION "0.1.0"

extern zend_module_entry study_module_entry;
#define phpext_study_ptr &study_module_entry

#ifdef PHP_WIN32
#	define PHP_STUDY_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_STUDY_API __attribute__ ((visibility("default")))
#else
#	define PHP_STUDY_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/**
 * Declare any global variables you may need between the BEGIN and END macros here
 */
ZEND_BEGIN_MODULE_GLOBALS(study)

ZEND_END_MODULE_GLOBALS(study)

void study_coroutine_util_init();
void study_coroutine_server_coro_init();

#endif	/* PHP_STUDY_H */