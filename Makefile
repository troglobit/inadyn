# Makefile for INADYN, a simple and small ddns client.          -*-Makefile-*-
# Use "make V=1" to see full GCC output

#VERSION      ?= $(shell git tag -l | tail -1)
VERSION      ?= 1.99.7-beta1
NAME          = inadyn
EXEC          = src/$(NAME)
PKG           = $(NAME)-$(VERSION)
ARCHIVE       = $(PKG).tar.bz2
MAN5          = inadyn.conf.5
MAN8          = inadyn.8

ROOTDIR      ?= $(dir $(shell pwd))
RM           ?= rm -f
CC           ?= $(CROSS)gcc
CHECK        := cppcheck $(CPPFLAGS) --quiet --enable=all

prefix       ?= /usr/local
sysconfdir   ?= /etc
datadir       = $(prefix)/share/doc/inadyn
mandir        = $(prefix)/share/man

# This magic trick looks like a comment, but works on BSD PMake
#include <config.mk>
include config.mk

PLUGIN_OBJS   = src/plugin.o		plugins/common.o	\
		plugins/dyndns.o	plugins/changeip.o	\
		plugins/dnsexit.o	plugins/easydns.o	\
		plugins/freedns.o	plugins/generic.o	\
		plugins/sitelutions.o	plugins/tunnelbroker.o	\
		plugins/tzo.o		plugins/zoneedit.o

BASE_OBJS     = src/main.o  src/ddns.o 	 src/cache.o	\
		src/error.o src/cmd.o    src/os.o	\
		src/http.o  src/tcp.o    src/ip.o       \
	        src/sha1.o  src/base64.o src/md5.o

OBJS	      = $(BASE_OBJS) $(PLUGIN_OBJS)
CFLAGS       ?= -O2 -W -Wall -Werror
CFLAGS       += $(CFG_INC) $(EXTRA_CFLAGS)
# -U_FORTIFY_SOURCE -- Disable annoying gcc warning for "warn_unused_result"
CPPFLAGS     += -U_FORTIFY_SOURCE -D_BSD_SOURCE -D_GNU_SOURCE
CPPFLAGS     += -Iinclude -DVERSION=\"$(VERSION)\"
LDFLAGS      ?=
LDLIBS       += -ldl
DISTFILES     = README COPYING LICENSE

# Pattern rules
.c.o:
	$(PRINTF) "  CC      $@\n"
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# Build rules
all: $(EXEC)

$(EXEC): $(OBJS)
	$(PRINTF) "  LINK    $@\n"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

install: $(EXEC)
	@install -d $(DESTDIR)$(prefix)/sbin
	@install -d $(DESTDIR)$(sysconfdir)
	@install -d $(DESTDIR)$(datadir)
	@install -d $(DESTDIR)$(mandir)/man8
	@install -d $(DESTDIR)$(mandir)/man5
	@install -m 0755 $(EXEC) $(DESTDIR)$(prefix)/sbin/
	@install -m 0644 man/$(MAN5) $(DESTDIR)$(mandir)/man5/$(MAN5)
	@install -m 0644 man/$(MAN8) $(DESTDIR)$(mandir)/man8/$(MAN8)
	@for file in $(DISTFILES); do \
		install -m 0644 $$file $(DESTDIR)$(datadir)/$$file; \
	done

uninstall:
	-@$(RM) $(DESTDIR)$(prefix)/sbin/$(EXEC)
	-@$(RM) -r $(DESTDIR)$(datadir)
	-@$(RM) $(DESTDIR)$(mandir)/man5/$(MAN5)
	-@$(RM) $(DESTDIR)$(mandir)/man8/$(MAN8)

clean:
	-@$(RM) $(OBJS) $(EXEC)

distclean:
	-@$(RM) $(OBJS) core $(EXEC) src/*~ include/*~ *~ src/*.o *.map src/.*.d *.out tags TAGS

check:
	$(CHECK) src/*.c

dist:
	@echo "Building bzip2 tarball of $(PKG) in parent dir..."
	git archive --format=tar --prefix=$(PKG)/ $(VERSION) | bzip2 >../$(ARCHIVE)
	@(cd ..; md5sum $(ARCHIVE) | tee $(ARCHIVE).md5)

