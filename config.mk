# Configuration options
EXTRA_CFLAGS    =
EXTRA_OBJS      =
EXTRA_CPPFLAGS  =
EXTRA_LIBS      =

# HTTPS support for update, if DDNS provider supports it.  To disable
# SSL support in Inadyn, set to 0 outside the Inadyn Makefile
ENABLE_SSL     ?= 1

# Default to use GnuTLS instead of OpenSSL, for the benefit of Debian
# and the GNU GPL license stipulations.
ifeq ($(ENABLE_SSL),1)
ifndef USE_OPENSSL
USE_GNUTLS    = 1
endif
endif

ifdef USE_GNUTLS
EXTRA_CPPFLAGS += -DENABLE_SSL -DCONFIG_GNUTLS
EXTRA_LIBS     += -lgnutls-openssl
endif

# This requires dynamic linking to OpenSSL and the libc crypto library
ifdef USE_OPENSSL
EXTRA_CPPFLAGS += -DENABLE_SSL -DCONFIG_OPENSSL
EXTRA_LIBS     += -lssl -lcrypto
endif

# Oversimplified arch setup, no smart detection.
# Possible values: linux, mac, solaris, yourown
ifndef TARGET_ARCH
TARGET_ARCH     = linux
endif

# Some targets, or older systems, may need -lresolv in EXTRA_LIBS
ifeq ($(TARGET_ARCH),solaris)
EXTRA_CFLAGS   +=
EXTRA_OBJS     +=
EXTRA_LIBS     += -lsocket -lnsl
endif
ifeq ($(TARGET_ARCH),linux)
EXTRA_CFLAGS   +=
EXTRA_OBJS     +=
EXTRA_LIBS     +=
endif

ifdef DEBUG
EXTRA_CFLAGS   += -g
endif

ifdef V
Q               =
MAKEFLAGS       =
PRINTF          = @true
else
Q               = @
MAKEFLAGS       = --silent --no-print-directory
PRINTF          = @printf
endif
