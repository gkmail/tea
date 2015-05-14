#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([tea], [1.0])

# Detect the canonical host and target build environment
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE

# Package version
# Making releases:
#   MICRO_VERSION += 1;
#   INTERFACE_AGE += 1;
#   BINARY_AGE += 1;
# if any functions have been added, set INTERFACE_AGE to 0.
# if backwards compatibility has been broken, set BINARY_AGE and INTERFACE_AGE to 0.

MAJOR_VERSION=0
MINOR_VERSION=1
MICRO_VERSION=2
INTERFACE_AGE=1
BINARY_AGE=1
VERSION=$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION

AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(INTERFACE_AGE)
AC_SUBST(BINARY_AGE)
AC_SUBST(VERSION)

# libtool versioning
LT_RELEASE=$MAJOR_VERSION.$MINOR_VERSION
LT_CURRENT=`expr $MICRO_VERSION - $INTERFACE_AGE`
LT_REVISION=$INTERFACE_AGE
LT_AGE=`expr $BINARY_AGE - $INTERFACE_AGE`

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

AC_CONFIG_SRCDIR([include/t_types.h])
AC_CONFIG_HEADERS([include/config.h])

CFLAGS="-Wall -Wno-unused-function -O3"

AC_ARG_ENABLE([memwatch], AS_HELP_STRING([--enable-memwatch], [use memwatch[default=no]]), , [enable_memwatch=no])
if test "x$enable_memwatch" = "xyes"; then
	CFLAGS="$CFLAGS -DMEMWATCH -DMEMWATCH_STDIO"
	AM_CONDITIONAL(ENABLE_MEMWATCH, true)
else
	AM_CONDITIONAL(ENABLE_MEMWATCH, false)
fi

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [debug mode[default=no]]), , [enable_debug=no])
if test "x$enable_debug" = "xyes"; then
	CFLAGS="$CFLAGS -g"
	AC_DEFINE_UNQUOTED(T_GLOBAL_DEBUG, 1, [Define if you want to enable debug information.])
fi


# Checks for programs.
AC_PROG_CC
AC_C_INLINE
AC_PROG_INSTALL
AC_CONFIG_MACRO_DIR([m4])
PKG_PROG_PKG_CONFIG()
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([inttypes.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

AC_OUTPUT([
	Makefile
	include/Makefile
	src/Makefile
	test/Makefile
])
