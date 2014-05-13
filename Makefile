# Makefile for INADYN, a simple and small ddns client.          -*-Makefile-*-
# Use "make V=1" to see full GCC output

#VERSION      ?= $(shell git tag -l | tail -1)
VERSION      ?= 1.99.7-beta2
NAME          = inadyn
PKG           = $(NAME)-$(VERSION)
DEV           = $(NAME)-dev
ARCHIVE       = $(PKG).tar.xz
EXEC          = src/$(NAME)
MAN5          = inadyn.conf.5
MAN8          = inadyn.8

ROOTDIR      ?= $(dir $(shell pwd))
RM           ?= rm -f
CC           ?= $(CROSS)gcc
CHECK        := cppcheck $(CPPFLAGS) --quiet --enable=all
INSTALL      := install --backup=off
STRIPINST    := $(INSTALL) -s --strip-program=$(CROSS)strip -m 0755

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

OBJS	      = $(BASE_OBJS) $(PLUGIN_OBJS) $(EXTRA_OBJS)
SRCS          = $(OBJS:.o=.c)
DEPS          = $(SRCS:.c=.d)

CFLAGS       ?= -O2 -W -Wall -Werror
CFLAGS       += $(CFG_INC) $(EXTRA_CFLAGS)
# -U_FORTIFY_SOURCE -- Disable annoying gcc warning for "warn_unused_result"
CPPFLAGS     += -U_FORTIFY_SOURCE -D_BSD_SOURCE -D_GNU_SOURCE
CPPFLAGS     += -Iinclude -DVERSION=\"$(VERSION)\"
LDFLAGS      ?=
LDLIBS       += -ldl $(EXTRA_LIBS)
DISTFILES     = README COPYING LICENSE

# Smart autodependecy generation via GCC -M.
%.d: %.c
	@$(SHELL) -ec "$(CC) -MM $(CFLAGS) $(CPPFLAGS) $< \
		| sed 's,.*: ,$*.o $@ : ,g' > $@; \
                [ -s $@ ] || rm -f $@"

# Pattern rules
.c.o:
	$(PRINTF) "  CC      $@\n"
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# Build rules
all: $(EXEC)

$(EXEC): $(OBJS)
	$(PRINTF) "  LINK    $@\n"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

strip:
	@$(CROSS)strip $(EXEC)
	@$(CROSS)size  $(EXEC)

install: $(EXEC)
	@$(INSTALL) -d $(DESTDIR)$(prefix)/sbin
	@$(INSTALL) -d $(DESTDIR)$(sysconfdir)
	@$(INSTALL) -d $(DESTDIR)$(datadir)
	@$(INSTALL) -d $(DESTDIR)$(mandir)/man8
	@$(INSTALL) -d $(DESTDIR)$(mandir)/man5
	@$(STRIPINST) $(EXEC) $(DESTDIR)$(prefix)/sbin/
	@$(INSTALL) -m 0644 man/$(MAN5) $(DESTDIR)$(mandir)/man5/$(MAN5)
	@$(INSTALL) -m 0644 man/$(MAN8) $(DESTDIR)$(mandir)/man8/$(MAN8)
	@for file in $(DISTFILES); do \
		$(INSTALL) -m 0644 $$file $(DESTDIR)$(datadir)/$$file; \
	done

uninstall:
	-@$(RM) $(DESTDIR)$(prefix)/sbin/`basename $(EXEC)`
	-@$(RM) -r $(DESTDIR)$(datadir)
	-@$(RM) $(DESTDIR)$(mandir)/man5/$(MAN5)
	-@$(RM) $(DESTDIR)$(mandir)/man8/$(MAN8)

clean:
	-@$(RM) $(OBJS) $(DEPS) $(EXEC)

distclean:
	-@$(RM) $(OBJS) $(DEPS) core $(EXEC) src/*~ *~ src/*.o src/*.d	\
		include/*~ plugins/*~ plugins/*.o plugins/*.d *.map 	\
			*.out tags TAGS

check:
	$(CHECK) $(SRCS)

dist: distclean
	@echo "Building xz tarball of $(PKG) in parent dir..."
	git archive --format=tar --prefix=$(PKG)/ $(VERSION) | xz >../$(ARCHIVE)
	@(cd ..; md5sum $(ARCHIVE) | tee $(ARCHIVE).md5)

dev: distclean
	@echo "Building unstable xz $(DEV) in parent dir..."
	-@$(RM) -f ../$(DEV).tar.xz*
	@(dir=`mktemp -d`; mkdir $$dir/$(DEV); cp -a . $$dir/$(DEV);	\
	  cd $$dir; tar --exclude=.git --exclude=contrib		\
			--exclude=inadyn.conf* --exclude=TODO		\
			-c -J -f $(DEV).tar.xz $(DEV);			\
	  cd - >/dev/null; mv $$dir/$(DEV).tar.xz ../; cd ..;		\
	  rm -rf $$dir; md5sum $(DEV).tar.xz | tee $(DEV).tar.xz.md5)

-include $(DEPS)
