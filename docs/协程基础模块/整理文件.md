# 整理文件

[仓库地址](https://github.com/php-extension-research/study)

在开发之前，我们先对项目的文件以及文件的内容进行整理，让结构更加的清晰。整理的依据是[开发规范](《PHP扩展开发》-协程-开发规范.md)。

首先，我们修改`php_study.h`文件的内容如下：

```cpp
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

#endif	/* PHP_STUDY_H */
```

然后，在`include`目录里面创建文件`study.h`，内容如下：

```cpp
#ifndef STUDY_H_
#define STUDY_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// include standard library
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>

#endif /* STUDY_H_ */
```

然后，我们再把`study.c`文件命名为`study.cc`，修改里面的内容为：

```cpp
#include "php_study.h"

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
```

因为，我们修改了`study.cc`的文件名字，所以我们需要修改`config.m4`里面的文件：

```shell
study_source_file="\
  study.cc \
  ${STUDY_ASM_DIR}make_${STUDY_CONTEXT_ASM_FILE} \
  ${STUDY_ASM_DIR}jump_${STUDY_CONTEXT_ASM_FILE}
"
```

把`study.c`改成了`study.cc`。

OK，整理完毕。

[下一篇：理解PHP生命周期的过程](./《PHP扩展开发》-协程-理解PHP生命周期的过程.md)

