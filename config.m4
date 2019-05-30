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
