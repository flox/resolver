# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

MAKEFILE_DIR ?= $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

# ---------------------------------------------------------------------------- #

.PHONY: all clean FORCE
.DEFAULT_GOAL = bin


# ---------------------------------------------------------------------------- #

CXX        ?= c++
RM         ?= rm -f
CAT        ?= cat
PKG_CONFIG ?= pkg-config
NIX        ?= nix
UNAME      ?= uname
MKDIR      ?= mkdir
MKDIR_P    ?= $(MKDIR) -p
CP         ?= cp
TR         ?= tr
SED        ?= sed
TEST       ?= test
DOXYGEN    ?= doxygen


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

VERSION := $(file < $(MAKEFILE_DIR)/version)


# ---------------------------------------------------------------------------- #

PREFIX     ?= $(MAKEFILE_DIR)/out
BINDIR     ?= $(PREFIX)/bin
LIBDIR     ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include


# ---------------------------------------------------------------------------- #

LIBFLOXRESOLVE = libflox-resolve$(libExt)

LIBS           = $(LIBFLOXRESOLVE)
COMMON_HEADERS = $(wildcard include/*.hh) $(wildcard include/flox/*.hh)
TESTS          = $(wildcard tests/*.cc)
SRCS           = $(wildcard src/*.cc)
bin_SRCS       = $(filter src/main%.cc, $(SRCS))
lib_SRCS       = $(filter-out $(bin_SRCS),$(SRCS))
BINS           = $(patsubst src/main-%.cc,%,$(bin_SRCS))


# ---------------------------------------------------------------------------- #

EXTRA_CXXFLAGS ?= -Wall -Wextra -Wpedantic
CXXFLAGS       ?= $(EXTRA_CFLAGS) $(EXTRA_CXXFLAGS)
CXXFLAGS       += '-I$(MAKEFILE_DIR)/include'
CXXFLAGS       += '-DFLOX_RESOLVER_VERSION="$(VERSION)"'
LDFLAGS        ?= $(EXTRA_LDFLAGS)
lib_CXXFLAGS   ?= -shared -fPIC
ifeq (Linux,$(OS))
lib_LDFLAGS  ?= -shared -fPIC -Wl,--no-undefined
else
lib_LDFLAGS  ?= -shared -fPIC -Wl,-undefined,error
endif
bin_CXXFLAGS   ?=
bin_LDFLAGS    ?=

ifneq ($(DEBUG),)
CXXFLAGS += -ggdb3 -pg
LDFLAGS  += -ggdb3 -pg
endif


# ---------------------------------------------------------------------------- #


sql_builder_CFLAGS ?=                                      \
  -I$(shell $(NIX) build --no-link --print-out-paths       \
                   '$(MAKEFILE_DIR)#sql-builder')/include
sql_builder_CFLAGS := $(sql_builder_CFLAGS)

# This provides `sqlite', `sqlite3pp', `argparse', `nix', `nlohmann_json'
pkgdb_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags flox-pkgdb)
pkgdb_CFLAGS := $(pkgdb_CFLAGS)
pkgdb_LDFLAGS ?= $(shell $(PKG_CONFIG) --libs flox-pkgdb)
pkgdb_LDFLAGS := $(pkgdb_CFLAGS)

ifndef floxresolve_LDFLAGS
floxresolve_LDFLAGS =  '-L$(MAKEFILE_DIR)/lib' -lflox-resolve
floxresolve_LDFLAGS += -Wl,--enable-new-dtags '-Wl,-rpath,$$ORIGIN/../lib'
endif


# ---------------------------------------------------------------------------- #

lib_CXXFLAGS += $(sql_builder_CFLAGS)
CXXFLAGS     += $(pkgdb_CFLAGS)

ifeq (Linux,$(OS))
LDFLAGS += -Wl,--as-needed
endif
LDFLAGS += $(pkgdb_LDFLAGS)
ifeq (Linux,$(OS))
LDFLAGS += -Wl,--no-as-needed
endif

bin_LDFLAGS += $(floxresolve_LDFLAGS)


# ---------------------------------------------------------------------------- #

SEMVER_PATH ?=                                                        \
  $(shell $(NIX) build --no-link --print-out-paths                    \
	                     'github:aakropotkin/floco#semver')/bin/semver
CXXFLAGS += -DSEMVER_PATH='$(SEMVER_PATH)'


# ---------------------------------------------------------------------------- #

.PHONY: bin lib include tests

bin:     $(addprefix bin/,$(BINS))
lib:     $(addprefix lib/,$(LIBS))
include: $(COMMON_HEADERS)
tests:   $(TESTS:.cc=)


# ---------------------------------------------------------------------------- #

clean: FORCE
	-$(RM) $(addprefix bin/,$(BINS))
	-$(RM) $(addprefix lib/,$(LIBS))
	-$(RM) src/*.o tests/*.o
	-$(RM) result
	-$(RM) -r $(PREFIX)
	-$(RM) $(addprefix docs/,*.png *.html *.svg *.css *.js)
	-$(RM) -r docs/search
	-$(RM) $(TESTS:.cc=)
	-$(RM) gmon.out *.log


# ---------------------------------------------------------------------------- #

%.o: %.cc $(COMMON_HEADERS)
	$(CXX) $(CXXFLAGS) -c "$<" -o "$@"


lib/$(LIBFLOXRESOLVE): CXXFLAGS += $(lib_CXXFLAGS)
lib/$(LIBFLOXRESOLVE): LDFLAGS  += $(lib_LDFLAGS)
lib/$(LIBFLOXRESOLVE): $(lib_SRCS:.cc=.o)
	$(MKDIR_P) $(@D)
	$(CXX) $^ $(LDFLAGS) -o "$@"


# ---------------------------------------------------------------------------- #

bin/%: CXXFLAGS += $(bin_CXXFLAGS)
bin/%: LDFLAGS  += $(bin_LDFLAGS)
$(addprefix bin/,$(BINS)): bin/%: src/main-%.o lib/$(LIBFLOXRESOLVE)
	$(MKDIR_P) $(@D)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) "$<" -o "$@"


# ---------------------------------------------------------------------------- #

.PHONY: install-dirs install-bin install-lib install-include install
install: install-dirs install-bin install-lib install-include

install-dirs: FORCE
	$(MKDIR_P) $(BINDIR) $(LIBDIR) $(INCLUDEDIR)/flox

$(INCLUDEDIR)/%: include/% | install-dirs
	$(CP) -- "$<" "$@"

$(LIBDIR)/%: lib/% | install-dirs
	$(CP) -- "$<" "$@"

$(BINDIR)/%: bin/% | install-dirs
	$(CP) -- "$<" "$@"

install-bin: $(addprefix $(BINDIR)/,$(BINS))
install-lib: $(addprefix $(LIBDIR)/,$(LIBS))
install-include:                                                    \
	$(addprefix $(INCLUDEDIR)/,$(subst include/,,$(COMMON_HEADERS)))


# ---------------------------------------------------------------------------- #

.PHONY: check

tests/%: CXXFLAGS += $(bin_CXXFLAGS)
tests/%: LDFLAGS  += $(bin_LDFLAGS)
$(TESTS:.cc=): %: %.o lib/$(LIBFLOXRESOLVE)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) "$<" -o "$@"


check: $(TESTS:.cc=)
	@_ec=0;                     \
	echo '';                    \
	for t in $(TESTS:.cc=); do  \
	  echo "Testing: $$t";      \
	  if "./$$t"; then          \
	    echo "PASS: $$t";       \
	  else                      \
	    _ec=1;                  \
	    echo "FAIL: $$t";       \
	  fi;                       \
	  echo '';                  \
	done;                       \
	exit "$$_ec"


# ---------------------------------------------------------------------------- #

all: bin lib tests


# ---------------------------------------------------------------------------- #

.PHONY: ccls
ccls: .ccls

.ccls: FORCE
	echo 'clang' > "$@";
	{                                                          \
	  if [[ -n "$(NIX_CC)" ]]; then                            \
	    $(CAT) "$(NIX_CC)/nix-support/libc-cflags";            \
	    $(CAT) "$(NIX_CC)/nix-support/libcxx-cxxflags";        \
	  fi;                                                      \
	  echo $(CXXFLAGS) $(pkgdb_CFLAGS) $(sql_builder_CFLAGS);  \
	}|$(TR) ' ' '\n'|$(SED) 's/-std=/%cpp -std=/' >> "$@";


# ---------------------------------------------------------------------------- #

.PHONY: docs

docs: docs/index.html

docs/index.html: FORCE
	$(DOXYGEN) ./Doxyfile


# ---------------------------------------------------------------------------- #


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
