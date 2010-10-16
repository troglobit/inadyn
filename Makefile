# INADYN Makefile

# This magic trick looks like a comment, but works on BSD PMake
#include <config.mk>
include config.mk

OUTFILE	      = $(OUTDIR)/$(TARGET_ARCH)/inadyn
BASE_OBJS     = $(OUTDIR)/base64utils.o $(OUTDIR)/dyndns.o $(OUTDIR)/errorcode.o		\
	        $(OUTDIR)/get_cmd.o $(OUTDIR)/http_client.o $(OUTDIR)/ip.o $(OUTDIR)/main.o	\
	        $(OUTDIR)/os_unix.o $(OUTDIR)/os_windows.o $(OUTDIR)/os.o $(OUTDIR)/os_psos.o	\
	        $(OUTDIR)/tcp.o $(OUTDIR)/inadyn_cmd.o
OBJS	      = $(BASE_OBJS) $(CFG_OBJ) $(EXTRA_OBJS)
CFLAGS       += $(CFG_INC) $(EXTRA_CFLAGS)
LDLIBS       += $(EXTRA_LIBS)

# Pattern rules
$(OUTDIR)/%.o : $(SRCDIR)/%.c
	@printf "  CC      $@\n"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# Build rules
all: $(OUTFILE)

$(OUTFILE): $(OUTDIR)/$(TARGET_ARCH)  $(OBJS)
	@printf "  LINK    $@\n"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(OUTDIR)/$(TARGET_ARCH):
	-@mkdir -p "$@"

# Rebuild this project
rebuild: cleanall all

# Clean this project
clean:
	-@rm -f $(OUTFILE)
	-@rm -f $(OBJS)

# Clean this project and all dependencies
cleanall: clean

