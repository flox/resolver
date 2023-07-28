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

CXXFLAGS     ?= $(EXTRA_CFLAGS) $(EXTRA_CXXFLAGS)
CXXFLAGS     += '-I$(MAKEFILE_DIR)/include'
LDFLAGS      ?= $(EXTRA_LDFLAGS)
lib_CXXFLAGS ?= -shared -fPIC
lib_LDFLAGS  ?= -shared -fPIC -Wl,--no-undefined
bin_CXXFLAGS ?=
bin_LDFLAGS  ?=

ifneq ($(DEBUG),)
	CXXFLAGS += -ggdb3 -pg
	LDFLAGS  += -ggdb3 -pg
endif


# ---------------------------------------------------------------------------- #

nljson_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags nlohmann_json)
nljson_CFLAGS := $(nljson_CFLAGS)

argparse_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags argparse)
argparse_CFLAGS := $(argparse_CFLAGS)

boost_CFLAGS    ?=                                                             \
  -I$(shell $(NIX) build --no-link --print-out-paths 'nixpkgs#boost')/include
boost_CFLAGS := $(boost_CFLAGS)

sqlite3_CFLAGS  ?= $(shell $(PKG_CONFIG) --cflags sqlite3)
sqlite3_CFLAGS  := $(sqlite3_CFLAGS)
sqlite3_LDFLAGS ?= $(shell $(PKG_CONFIG) --libs sqlite3)
sqlite3_LDLAGS  := $(sqlite3_LDLAGS)

sql_builder_CFLAGS ?=                                      \
  -I$(shell $(NIX) build --no-link --print-out-paths       \
                   '$(MAKEFILE_DIR)#sql-builder')/include
sql_builder_CFLAGS := $(sql_builder_CFLAGS)

nix_INCDIR  ?= $(shell $(PKG_CONFIG) --variable=includedir nix-cmd)
nix_INCDIR  := $(nix_INCDIR)
ifndef nix_CFLAGS
  nix_CFLAGS  =  $(boost_CFLAGS)
  nix_CFLAGS  += $(shell $(PKG_CONFIG) --cflags nix-main nix-cmd nix-expr)
  nix_CFLAGS  += -isystem $(shell $(PKG_CONFIG) --variable=includedir nix-cmd)
  nix_CFLAGS  += -include $(nix_INCDIR)/nix/config.h
endif
nix_CFLAGS := $(nix_CFLAGS)

ifndef nix_LDFLAGS
  nix_LDFLAGS =                                                        \
	  $(shell $(PKG_CONFIG) --libs nix-main nix-cmd nix-expr nix-store)
  nix_LDFLAGS += -lnixfetchers
endif
nix_LDFLAGS := $(nix_LDFLAGS)

ifndef floxresolve_LDFLAGS
	floxresolve_LDFLAGS =  '-L$(MAKEFILE_DIR)/lib' -lflox-resolve
	floxresolve_LDFLAGS += -Wl,--enable-new-dtags '-Wl,-rpath,$$ORIGIN/../lib'
endif


# ---------------------------------------------------------------------------- #

lib_CXXFLAGS += $(sqlite3_CFLAGS) $(sql_builder_CFLAGS)
bin_CXXFLAGS += $(argparse_CFLAGS)
CXXFLAGS     += $(nix_CFLAGS) $(nljson_CFLAGS)

lib_LDFLAGS += -Wl,--as-needed
lib_LDFLAGS += $(nix_LDFLAGS) $(sqlite3_LDFLAGS)
lib_LDFLAGS += -Wl,--no-as-needed

bin_LDFLAGS += $(nix_LDFLAGS) $(floxresolve_LDFLAGS)


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
	-$(RM) -r doc/html
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

tests/%: CXXFLAGS += $(sqlite3_CFLAGS) $(nljson_CFLAGS)
tests/%: CXXFLAGS += $(nix_CFLAGS) $(nljson_CFLAGS) $(bin_CXXFLAGS)
tests/%: LDFLAGS  += $(floxresolve_LDFLAGS) $(bin_LDFLAGS)
tests/%: LDFLAGS  += $(sqlite3_LDFLAGS) $(nix_LDFLAGS)
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

.ccls: FORCE
	echo 'clang' > "$@";
	{                                                                     \
	  if [[ -n "$(NIX_CC)" ]]; then                                       \
	    $(CAT) "$(NIX_CC)/nix-support/libc-cflags";                       \
	    $(CAT) "$(NIX_CC)/nix-support/libcxx-cxxflags";                   \
	  fi;                                                                 \
	  echo $(CXXFLAGS) $(sqlite3_CFLAGS) $(nljson_CFLAGS) $(nix_CFLAGS);  \
	  echo $(nljson_CFLAGS) $(argparse_CFLAGS) $(sql_builder_CFLAGS);     \
	}|$(TR) ' ' '\n'|$(SED) 's/-std=/%cpp -std=/' >> "$@";


# ---------------------------------------------------------------------------- #

.PHONY: doc

doc: doc/html/index.html

doc/html/index.html: FORCE
	$(DOXYGEN) ./Doxyfile


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
