# Oversimplified arch setup, no smart detection.
# Possible values: linux, mac, solaris, yourown
ifndef TARGET_ARCH
TARGET_ARCH   = linux
endif

# Some targets, or older systems, may need -lresolv in EXTRA_LIBS
ifeq ($(TARGET_ARCH),solaris)
EXTRA_CFLAGS  =
EXTRA_OBJS    =
EXTRA_LIBS    = -lsocket -lnsl
endif
ifeq ($(TARGET_ARCH),linux)
EXTRA_CFLAGS  =
EXTRA_OBJS    =
EXTRA_LIBS    =
endif

ifdef DEBUG
EXTRA_CFLAGS += -g
endif

ifdef V
Q             =
MAKEFLAGS     =
PRINTF        = @true
else
Q             = @
MAKEFLAGS     = --silent --no-print-directory
PRINTF        = @printf
endif
