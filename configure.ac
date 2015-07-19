#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)
AC_INIT([fatutil], [0.1], [joshua.henderson@linux.com])
AM_INIT_AUTOMAKE([foreign dist-bzip2 1.10])
AC_CONFIG_SRCDIR([fatutil.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h sys/statvfs.h sys/vfs.h unistd.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_STRUCT_TM
AC_TYPE_UINT16_T

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_CHECK_FUNCS([memset strrchr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
