#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(inadyn, 1.99.13, https://github.com/troglobit/inadyn/issues)
AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-xz])
AM_SILENT_RULES()

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([include/config.h])
AC_CONFIG_FILES([Makefile src/Makefile include/Makefile man/Makefile])

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

# If HTTPS is enabled we check for either OpenSSL or GnuTLS libs+headers
if test "$ac_enable_ssl" = "yes"; then
   if test "$ac_enable_openssl" = "yes"; then
      AC_CHECK_LIB([crypto], [EVP_EncryptInit], [],
                          AC_MSG_ERROR([*** Crypto library (OpenSSL) not found!]))
      AC_CHECK_LIB([ssl], [SSL_library_init], [],
                          AC_MSG_ERROR([*** SSL library (OpenSSL) not found!]))
      AC_CHECK_HEADERS([openssl/crypto.h openssl/x509.h openssl/pem.h openssl/ssl.h \
                        openssl/tls1.h openssl/err.h])
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

AC_OUTPUT
