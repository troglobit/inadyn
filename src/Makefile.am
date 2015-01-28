## Inadyn - A small and simple DDNS client                -*-Makefile-*-

AUTOMAKE_OPTIONS= subdir-objects
sbin_PROGRAMS	= inadyn
inadyn_SOURCES	= main.c	ddns.c		cache.c		\
		  error.c	cmd.c		os.c		\
		  strlcpy.c	strlcat.c	http.c		\
		  ssl.c		tcp.c		ip.c		\
	          sha1.c	base64.c	md5.c		\
		  plugin.c

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
		  ../plugins/dtdns.c
inadyn_LDADD	= -ldl
