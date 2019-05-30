---
title: 《PHP扩展开发》--协程(编写config.m4文件)
date: 2019-05-30 13:51:26
tags:
- PHP
- PHP扩展
- Swoole
---

[仓库地址](https://github.com/php-extension-research/study)

这是一个系列的文章，会逐步带大家去实现一个`PHP`协程扩展。我们把这个扩展叫做`study`。

首先，我们需要去生成扩展的基本目录。我们可以在`PHP`的源码里面找到一个工具叫做`ext_skel`。它可以帮我们生成扩展目录。这里不进行介绍。

生成扩展目录后，我们需要去复制一份`Swoole`扩展目录下的`thirdparty/boost`。因为我们写上下文切换的时候，会用到这些库：

```shell
~/codeDir/cppCode/study # tree
.
├── CREDITS
├── EXPERIMENTAL
├── config.m4
├── include
├── make.sh
├── php_study.h
├── study.c
├── study.php
├── tests
│   └── 001.phpt
└── thirdparty
    └── boost
        └── asm
            ├── jump_arm64_aapcs_elf_gas.S
            ├── jump_arm64_aapcs_macho_gas.S
            ├── jump_arm_aapcs_elf_gas.S
            ├── jump_arm_aapcs_macho_gas.S
            ├── jump_arm_aapcs_pe_armasm.asm
            ├── jump_combined_sysv_macho_gas.S
            ├── jump_i386_ms_pe_gas.asm
            ├── jump_i386_ms_pe_masm.asm
            ├── jump_i386_sysv_elf_gas.S
            // 省略了其他的文件
```

然后，我们打开`config.m4`文件，进行编写：

```shell
PHP_ARG_ENABLE(study, whether to enable study support,
Make sure that the comment is aligned:
[  --enable-study           Enable study support])

# AC_CANONICAL_HOST

if test "$PHP_STUDY" != "no"; then
    PHP_ADD_LIBRARY(pthread)
    STUDY_ASM_DIR="thirdparty/boost/asm/"
    CFLAGS="-Wall -pthread $CFLAGS"

    AS_CASE([$host_os],
      [linux*], [STUDY_OS="LINUX"],
      []
    )

    AS_CASE([$host_cpu],
      [x86_64*], [STUDY_CPU="x86_64"],
      [x86*], [STUDY_CPU="x86"],
      [i?86*], [STUDY_CPU="x86"],
      [arm*], [STUDY_CPU="arm"],
      [aarch64*], [STUDY_CPU="arm64"],
      [arm64*], [STUDY_CPU="arm64"],
      []
    )

    if test "$STUDY_CPU" = "x86_64"; then
        if test "$STUDY_OS" = "LINUX"; then
            STUDY_CONTEXT_ASM_FILE="x86_64_sysv_elf_gas.S"
        fi
    elif test "$STUDY_CPU" = "x86"; then
        if test "$STUDY_OS" = "LINUX"; then
            STUDY_CONTEXT_ASM_FILE="i386_sysv_elf_gas.S"
        fi
    elif test "$STUDY_CPU" = "arm"; then
        if test "$STUDY_OS" = "LINUX"; then
            STUDY_CONTEXT_ASM_FILE="arm_aapcs_elf_gas.S"
        fi
    elif test "$STUDY_CPU" = "arm64"; then
        if test "$STUDY_OS" = "LINUX"; then
            STUDY_CONTEXT_ASM_FILE="arm64_aapcs_elf_gas.S"
        fi
    elif test "$STUDY_CPU" = "mips32"; then
        if test "$STUDY_OS" = "LINUX"; then
           STUDY_CONTEXT_ASM_FILE="mips32_o32_elf_gas.S"
        fi
    fi

    study_source_file="\
        study.c \
        ${STUDY_ASM_DIR}make_${STUDY_CONTEXT_ASM_FILE} \
        ${STUDY_ASM_DIR}jump_${STUDY_CONTEXT_ASM_FILE}
    "

    PHP_NEW_EXTENSION(study, $study_source_file, $ext_shared, ,, cxx)

    PHP_ADD_INCLUDE([$ext_srcdir])
    PHP_ADD_INCLUDE([$ext_srcdir/include])

    PHP_INSTALL_HEADERS([ext/study], [*.h config.h include/*.h thirdparty/*.h])

    PHP_REQUIRE_CXX()

    CXXFLAGS="$CXXFLAGS -Wall -Wno-unused-function -Wno-deprecated -Wno-deprecated-declarations"
    CXXFLAGS="$CXXFLAGS -std=c++11"

    PHP_ADD_BUILD_DIR($ext_builddir/thirdparty/boost)
    PHP_ADD_BUILD_DIR($ext_builddir/thirdparty/boost/asm)
fi

```

内容很长，我们慢慢来看：

```shell
AS_CASE([$host_os],
  [linux*], [STUDY_OS="LINUX"],
  []
)
```

这段是用来判断我们机器所使用的操作系统是什么类型的，然后把操作系统的类型赋值给变量`STUDY_OS`。因为，我们的这个扩展只打算支持`Linux`，所以，我这个里面只写了`Linux`。

```shell
AS_CASE([$host_cpu],
  [x86_64*], [STUDY_CPU="x86_64"],
  [x86*], [STUDY_CPU="x86"],
  [i?86*], [STUDY_CPU="x86"],
  [arm*], [STUDY_CPU="arm"],
  [aarch64*], [STUDY_CPU="arm64"],
  [arm64*], [STUDY_CPU="arm64"],
  []
)
```

类似的，这段是判断`CPU`的类型，然后赋值给变量`STUDY_CPU`。

```shell
if test "$STUDY_CPU" = "x86_64"; then
  if test "$STUDY_OS" = "LINUX"; then
  	STUDY_CONTEXT_ASM_FILE="x86_64_sysv_elf_gas.S"
  fi
elif test "$STUDY_CPU" = "x86"; then
  if test "$STUDY_OS" = "LINUX"; then
  	STUDY_CONTEXT_ASM_FILE="i386_sysv_elf_gas.S"
  fi
elif test "$STUDY_CPU" = "arm"; then
  if test "$STUDY_OS" = "LINUX"; then
  	STUDY_CONTEXT_ASM_FILE="arm_aapcs_elf_gas.S"
  fi
elif test "$STUDY_CPU" = "arm64"; then
  if test "$STUDY_OS" = "LINUX"; then
  	STUDY_CONTEXT_ASM_FILE="arm64_aapcs_elf_gas.S"
  fi
elif test "$STUDY_CPU" = "mips32"; then
  if test "$STUDY_OS" = "LINUX"; then
  	STUDY_CONTEXT_ASM_FILE="mips32_o32_elf_gas.S"
  fi
fi
```

这段是判断应该用什么类型的汇编文件，然后赋值给变量`STUDY_CONTEXT_ASM_FILE`。

```shell
study_source_file="\
  study.c \
  ${STUDY_ASM_DIR}make_${STUDY_CONTEXT_ASM_FILE} \
  ${STUDY_ASM_DIR}jump_${STUDY_CONTEXT_ASM_FILE}
"
```

这段是把我们需要编译的所有文件已字符串的方式存入到变量`study_source_file`里面。

```shell
PHP_NEW_EXTENSION(study, $study_source_file, $ext_shared, ,, cxx)
```

这段是声明这个扩展的名称、需要的源文件名、此扩展的编译形式。其中`$ext_shared`代表此扩展是动态库，使用`cxx`的原因是，我们的这个扩展使用`C++`来编写。

```shell
PHP_ADD_INCLUDE([$ext_srcdir])
PHP_ADD_INCLUDE([$ext_srcdir/include])
```

这段是用来添加额外的包含头文件的目录。

```shell
PHP_INSTALL_HEADERS([ext/study], [*.h config.h include/*.h thirdparty/*.h])
```

这段是把我们的`study`扩展目录里面的`*.h`、`config.h`、`include/*.h`、`thirdparty/*.h`复制到：

```shell
php-config --include-dir
```

下的`ext/study`里面。这个是在执行`make install`的时候会进行复制。我们待会会看到。

```shell
PHP_REQUIRE_CXX()
```

因为，我们使用了`C++`，所以我们需要指明一下。（没有这句会编译出错）

```shell
CXXFLAGS="$CXXFLAGS -Wall -Wno-unused-function -Wno-deprecated -Wno-deprecated-declarations"
CXXFLAGS="$CXXFLAGS -std=c++11"
```

这段是指定编译`C++`时候，用到的编译选项。

```shell
PHP_ADD_BUILD_DIR($ext_builddir/thirdparty/boost)
PHP_ADD_BUILD_DIR($ext_builddir/thirdparty/boost/asm)
```

这段是指定这个扩展需要被编译到的文件的目录。因为我们需要编译`boost`提供的代码，所以需要进行指定。

编写完之后，我们就可以进行编译了：

```shell
~/codeDir/cppCode/study # ./make.sh 
```

（如果无法执行`make.sh`脚本，需要设置它为可执行）

然后，会看到以下输出：

```shell
----------------------------------------------------------------------

Build complete.
Don't forget to run 'make test'.

Installing shared extensions:     /usr/local/lib/php/extensions/no-debug-non-zts-20180731/
Installing header files:          /usr/local/include/php/
~/codeDir/cppCode/study # 
```

代表编译成功了。`study.so`扩展被存放在了：

```shell
/usr/local/lib/php/extensions/no-debug-non-zts-20180731/
```

里面。

OK，我们现在来看看我们扩展的头文件是否被复制了：

```shell
~/codeDir/cppCode/study # ls /usr/local/include/php/ext/study/
config.h     include      php_study.h  thirdparty
~/codeDir/cppCode/study # 
```

（本文结束）







