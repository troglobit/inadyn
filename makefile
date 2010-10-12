INADYN_BASE = .
SRCDIR = $(INADYN_BASE)/src
OUTDIR=bin

#simple arch config. No smart detection.
#possible values: linux, mac, solaris, yourown
ifndef TARGET_ARCH
	TARGET_ARCH = linux
endif

OUTFILE=$(OUTDIR)/$(TARGET_ARCH)/inadyn

# Make command to use for dependencies
MAKECMD=gmake

ifeq ($(TARGET_ARCH),solaris)
	ARCH_SPECIFIC_LIBS += -lsocket -lnsl
	ARCH_SPECIFIC_CFLAGS += 
endif

ifeq ($(TARGET_ARCH),linux)
	ARCH_SPECIFIC_LIBS += 
	ARCH_SPECIFIC_CFLAGS += 
endif

ifdef DEBUG
	CFLAGS += -g
endif

COMMON_OBJ=$(OUTDIR)/base64utils.o $(OUTDIR)/dyndns.o \
	$(OUTDIR)/errorcode.o $(OUTDIR)/get_cmd.o $(OUTDIR)/http_client.o \
	$(OUTDIR)/ip.o $(OUTDIR)/main.o $(OUTDIR)/os.o $(OUTDIR)/os_psos.o \
	$(OUTDIR)/os_unix.o $(OUTDIR)/os_windows.o $(OUTDIR)/tcp.o $(OUTDIR)/inadyn_cmd.o
OBJ=$(COMMON_OBJ) $(CFG_OBJ)

COMPILE=gcc  -Wall  -pedantic -c  $(ARCH_SPECIFIC_CFLAGS) $(CFLAGS) -o "$(OUTDIR)/$(*F).o" $(CFG_INC) "$<"
LINK=gcc $(CFLAGS) -o "$(OUTFILE)" $(OBJ) $(CFG_LIB) $(ARCH_SPECIFIC_LIBS)

# Pattern rules
$(OUTDIR)/%.o : $(SRCDIR)/%.c
	$(COMPILE)

# Build rules
all: $(OUTFILE)

$(OUTFILE): $(OUTDIR)/$(TARGET_ARCH)  $(OBJ)
	$(LINK)

$(OUTDIR)/$(TARGET_ARCH):
	-mkdir -p "$@"

# Rebuild this project
rebuild: cleanall all

# Clean this project
clean:
	rm -f $(OUTFILE)
	rm -f $(OBJ)

# Clean this project and all dependencies
cleanall: clean

