#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(inadyn, 2.0.0-beta3, https://github.com/troglobit/inadyn/issues)
AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([include/config.h])
AC_CONFIG_FILES([Makefile libite/Makefile src/Makefile include/Makefile man/Makefile])
AC_CONFIG_MACRO_DIR([m4])

AC_ARG_ENABLE(ssl,
        [AS_HELP_STRING([--disable-ssl], [Disable HTTPS support, default: enabled])],
        [ac_enable_ssl="$enableval"],
        [ac_enable_ssl="yes"]
)

AC_ARG_ENABLE(openssl,
        [AS_HELP_STRING([--enable-openssl], [Use OpenSSL for HTTPS, default: GnuTLS])],
        [ac_enable_openssl="$enableval"],
        [ac_enable_openssl="no"]
)

# Define necessary build flags
AC_GNU_SOURCE
AC_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h arpa/nameser.h netinet/in.h stdlib.h string.h \
                  sys/ioctl.h sys/socket.h sys/types.h syslog.h unistd.h],
                  [], [],
[
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

])

# Needed for the libraries
AM_PROG_AR
LT_INIT([disable-shared static])

# Check for libConfuse
AC_CHECK_LIB([confuse], [cfg_init], [],
             AC_MSG_ERROR([*** Configuration file parser library (libConfuse) not found!]))
AC_CHECK_HEADERS([confuse.h])

# If HTTPS is enabled we check for either OpenSSL or GnuTLS libs+headers
if test "$ac_enable_ssl" = "yes"; then
   if test "$ac_enable_openssl" = "yes"; then
      AC_CHECK_LIB([crypto], [EVP_EncryptInit], [],
                          AC_MSG_ERROR([*** Crypto library (OpenSSL) not found!]))
      AC_CHECK_LIB([ssl], [SSL_library_init], [],
                          AC_MSG_ERROR([*** SSL library (OpenSSL) not found!]))
      AC_CHECK_HEADERS([openssl/crypto.h openssl/x509.h openssl/pem.h openssl/ssl.h \
                        openssl/tls1.h openssl/err.h], [], [],
			[
			#include <openssl/conf.h>
			])
      AC_DEFINE([CONFIG_OPENSSL], [], [Enable HTTPS support using OpenSSL library])
   else
      AC_CHECK_LIB([gnutls], [gnutls_init], [],
                          AC_MSG_ERROR([*** SSL library (GnuTLS) not found!]))
      AC_CHECK_LIB([gnutls-openssl], [SSL_library_init], [],
                          AC_MSG_ERROR([*** Compat OpenSSL library (GnuTLS) not found!]))
      AC_CHECK_HEADERS([gnutls/openssl.h])
      AC_DEFINE([CONFIG_GNUTLS], [], [Enable HTTPS support using GnuTLS library])
   fi
   AC_DEFINE([ENABLE_SSL], [], [Enable HTTPS support])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([atexit memset poll socket strerror])

# Check if user has downloaded a GitHub ZIP file and attempt to fix it.
AC_MSG_CHECKING([status of GIT submodules])
if test ! -e libite/lite.h; then
    AC_MSG_RESULT([Missing!])
    AC_MSG_CHECKING([if we can update all submodules])
    run=`git submodule update --init`
    if test $? -ne 0; then
       AC_MSG_RESULT([Failed!  No Internet access?])
       exit 1
    fi
    AC_MSG_RESULT([Yes, all fixed!])
else
    AC_MSG_RESULT([OK])
fi

AC_OUTPUT
