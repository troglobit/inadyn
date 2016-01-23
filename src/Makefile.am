AUTOMAKE_OPTIONS= subdir-objects
sbin_PROGRAMS	= inadyn
inadyn_SOURCES	= main.c	ddns.c		cache.c		\
		  error.c	conf.c		os.c		\
		  http.c	plugin.c	tcp.c		\
		  ip.c		sha1.c		base64.c	\
		  md5.c
if ENABLE_SSL
if ENABLE_OPENSSL
inadyn_SOURCES += openssl.c
else
inadyn_SOURCES += gnutls.c
endif
endif

## Plugins are currently built-in, and built from this directory instead
## of where they reside.  They should be built by plugins/Makefile.am
## and be installed into $libdir/inadyn/plugins/ as *.so files
inadyn_SOURCES += ../plugins/common.c		../plugins/dyndns.c	\
		  ../plugins/changeip.c		../plugins/dnsexit.c	\
		  ../plugins/easydns.c		../plugins/freedns.c	\
		  ../plugins/generic.c		../plugins/sitelutions.c\
		  ../plugins/tunnelbroker.c	../plugins/tzo.c	\
		  ../plugins/zoneedit.c		../plugins/zerigo.c	\
		  ../plugins/dhis.c		../plugins/duckdns.c	\
		  ../plugins/dtdns.c		../plugins/giradns.c	\
		  ../plugins/duiadns.c		../plugins/dynv6.c	\
		  ../plugins/dynv6-ipv4.c
inadyn_CFLAGS   = -W -Wall -Wextra -std=gnu99 $(libite_CFLAGS)
inadyn_LDADD    = -ldl $(libite_LIBS)
inadyn_DEPENDENCIES=

if BUILTIN_LITE
inadyn_DEPENDENCIES += ../libite/libite.la
endif

# Slightly ugly hacks to prevent the build system from installing
# builtins, which it does if we list GIT submodules in SUBDIRS.
$(inadyn_DEPENDENCIES):
	@$(MAKE) -C `dirname $@` all

clean-local:
	-@for dep in $(inadyn_DEPENDENCIES); do		\
		$(MAKE) -C `dirname $$dep` clean;	\
	done
