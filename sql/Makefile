# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

.PHONY: all clean check FORCE
.DEFAULT_GOAL = all


# ---------------------------------------------------------------------------- #

PKG_CONFIG ?= pkg-config
UNAME      ?= uname
SQLITE3    ?= sqlite3
GREP       ?= grep
MKDIR      ?= mkdir
MKDIR_P    ?= $(MKDIR) -p
CP         ?= cp


# ---------------------------------------------------------------------------- #

sqlite3_CFLAGS  ?= $(shell pkg-config --cflags sqlite3; )
sqlite3_LDFLAGS ?= $(shell pkg-config --cflags sqlite3; )


# ---------------------------------------------------------------------------- #

CFLAGS ?=
CFLAGS += -shared -fPIC -O2 $(sqlite3_CFLAGS)

LDFLAGS ?=
LDFLAGS += -shared $(sqlite3_LDFLAGS) -Wl,--no-undefined
LDFLAGS += -Wl,--enable-new-dtags '-Wl,-rpath,$$ORIGIN/../lib'

ifneq (,$(DEBUG))
CFLAGS  += -ggdb3 -pg
LDFLAGS += -ggdb3 -pg
endif  # ifneq (,$(DEBUG))


# ---------------------------------------------------------------------------- #

OS ?= $(shell $(UNAME))
OS := $(OS)
ifndef libExt
ifeq (Linux,$(OS))
	libExt ?= .so
else
	libExt ?= .dylib
endif  # ifeq (Linux,$(OS))
endif  # ifndef libExt


# ---------------------------------------------------------------------------- #

LIBS = libsqlexts$(libExt)

.PHONY: lib
lib: $(LIBS)
all: lib

libsqlexts$(libExt): hash_str.o
	$(LINK.c) $(filter %.o,$^) $(LDLIBS) -o "$@"



# ---------------------------------------------------------------------------- #

clean: FORCE
	-$(RM) *.o $(LIBS)
	-$(RM) ./.tmpdb.db


# ---------------------------------------------------------------------------- #

check: libsqlexts$(libExt)
	-$(RM) ./.tmpdb.db
	$(SQLITE3) ./.tmpdb.db ".load ./libsqlexts"      \
	           "SELECT hash_str( 'Hello, World!' )"  \
	  |$(GREP) '^-6390844608310610124$$'


# ---------------------------------------------------------------------------- #

PREFIX ?= out
.phony: install
install: $(LIBS)
	$(MKDIR_P) $(PREFIX)/libexec
	$(CP) -t $(PREFIX)/libexec $(LIBS)


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
