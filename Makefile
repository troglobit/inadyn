# Makefile for INADYN, a simple and small ddns client.          -*-Makefile-*-

#VERSION      ?= $(shell git tag -l | tail -1)
VERSION      ?= 1.99.2
NAME          = inadyn
EXEC          = src/$(NAME)
PKG           = $(NAME)-$(VERSION)
ARCHIVE       = $(PKG).tar.bz2
MAN5          = inadyn.conf.5
MAN8          = inadyn.8

ROOTDIR      ?= $(dir $(shell pwd))
RM           ?= rm -f
CC           ?= $(CROSS)gcc

prefix       ?= /usr/local
sysconfdir   ?= /etc
datadir       = $(prefix)/share/doc/inadyn
mandir        = $(prefix)/share/man

# This magic trick looks like a comment, but works on BSD PMake
#include <config.mk>
include config.mk

BASE_OBJS     = src/base64utils.o src/md5.o src/dyndns.o src/errorcode.o src/get_cmd.o \
		src/http_client.o src/ip.o src/main.o src/os_unix.o src/os_windows.o   \
		src/os.o src/os_psos.o src/tcp.o src/inadyn_cmd.o
OBJS	      = $(BASE_OBJS) $(CFG_OBJ) $(EXTRA_OBJS)
CFLAGS        = -Iinclude -DVERSION_STRING=\"$(VERSION)\" $(CFG_INC) $(EXTRA_CFLAGS)
CFLAGS       += -O2 -W -Wall
LDLIBS       += -lresolv $(EXTRA_LIBS)
DISTFILES     = README COPYING LICENSE

# Pattern rules
.c.o:
	@printf "  CC      $@\n"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# Build rules
all: $(EXEC)

$(EXEC): $(OBJS)
	@printf "  LINK    $@\n"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

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
	-@$(RM) $(OBJS) core $(EXEC) *.o *.map .*.d *.out tags TAGS

dist:
	@echo "Building bzip2 tarball of $(PKG) in parent dir..."
	git archive --format=tar --prefix=$(PKG)/ $(VERSION) | bzip2 >../$(ARCHIVE)
	@(cd ..; md5sum $(ARCHIVE) | tee $(ARCHIVE).md5)

