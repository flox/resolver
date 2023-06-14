# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

MAKEFILE_DIR ?= $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

# ---------------------------------------------------------------------------- #

.PHONY: all clean FORCE
.DEFAULT_GOAL = all


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


# ---------------------------------------------------------------------------- #

OS ?= $(shell $(UNAME))
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

BINS           =  resolver
LIBS           =  $(LIBFLOXRESOLVE)
COMMON_HEADERS =  resolve.hh descriptor.hh flox/exceptions.hh flox/types.hh
COMMON_HEADERS += flox/util.hh semver.hh flox/package.hh
TESTS          =  $(wildcard tests/*.cc)


# ---------------------------------------------------------------------------- #

CXXFLAGS     ?=
CXXFLAGS     += '-I$(MAKEFILE_DIR)/include'
lib_CXXFLAGS =  -shared -fPIC
lib_LDFLAGS  =  -shared -fPIC -Wl,--no-undefined
bin_CXXFLAGS =
bin_LDFLAGS  =

ifneq ($(DEBUG),)
	CXXFLAGS += -ggdb3 -pg
	LDFLAGS  += -ggdb3 -pg
endif

nljson_CFLAGS   =  $(shell $(PKG_CONFIG) --cflags nlohmann_json)
argparse_CFLAGS =  $(shell $(PKG_CONFIG) --cflags argparse)
boost_CFLAGS    ?=                                                             \
  -I$(shell $(NIX) build --no-link --print-out-paths 'nixpkgs#boost')/include

sqlite3_CFLAGS  = $(shell $(PKG_CONFIG) --cflags sqlite3)
sqlite3_LDFLAGS =  $(shell $(PKG_CONFIG) --libs sqlite3)

nix_INCDIR  =  $(shell $(PKG_CONFIG) --variable=includedir nix-cmd)
nix_CFLAGS  =  $(boost_CFLAGS)
nix_CFLAGS  += $(shell $(PKG_CONFIG) --cflags nix-main nix-cmd nix-expr)
nix_CFLAGS  += -isystem $(shell $(PKG_CONFIG) --variable=includedir nix-cmd)
nix_CFLAGS  += -include $(nix_INCDIR)/nix/config.h
nix_LDFLAGS =  $(shell $(PKG_CONFIG) --libs nix-main nix-cmd nix-expr nix-store)
nix_LDFLAGS += -lnixfetchers


floxresolve_LDFLAGS =  '-L$(MAKEFILE_DIR)/lib' -lflox-resolve
floxresolve_LDFLAGS += -Wl,--enable-new-dtags '-Wl,-rpath,$$ORIGIN/../lib'


# ---------------------------------------------------------------------------- #

bin_CXXFLAGS += $(argparse_CFLAGS)


# ---------------------------------------------------------------------------- #

HAVE_INSTALLABLE_FLAKE =                                                   \
	$(shell $(TEST) -r $(nix_INCDIR)/nix/installable-flake.hh && echo 1||:)
ifeq (,$(HAVE_INSTALLABLE_FLAKE))
  CXXFLAGS += -UHAVE_INSTALLABLE_FLAKE
else
	CXXFLAGS += -DHAVE_INSTALLABLE_FLAKE='$(HAVE_INSTALLABLE_FLAKE)'
endif


# ---------------------------------------------------------------------------- #

SEMVER_PATH ?=                                                        \
  $(shell $(NIX) build --no-link --print-out-paths                    \
	                     'github:aakropotkin/floco#semver')/bin/semver
CXXFLAGS += -DSEMVER_PATH='$(SEMVER_PATH)'


# ---------------------------------------------------------------------------- #

.PHONY: bin lib include

bin: $(addprefix bin/,$(BINS))
lib: $(addprefix lib/,$(LIBS))
include: $(addprefix include/,$(COMMON_HEADERS))


# ---------------------------------------------------------------------------- #

clean: FORCE
	-$(RM) $(addprefix bin/,$(BINS))
	-$(RM) $(addprefix lib/,$(LIBS))
	-$(RM) src/*.o
	-$(RM) result
	-$(RM) -r $(PREFIX)
	-$(RM) tests/$(TESTS:.cc=)


# ---------------------------------------------------------------------------- #

src/%.o: $(addprefix include/,$(COMMON_HEADERS))


# ---------------------------------------------------------------------------- #

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c "$<"


lib/$(LIBFLOXRESOLVE): CXXFLAGS += $(lib_CXXFLAGS) $(nix_CFLAGS)
lib/$(LIBFLOXRESOLVE): CXXFLAGS += $(nljson_CFLAGS) $(sqlite3_CFLAGS)
lib/$(LIBFLOXRESOLVE): LDFLAGS  += $(lib_LDFLAGS)
lib/$(LIBFLOXRESOLVE): LDFLAGS  += -Wl,--as-needed
lib/$(LIBFLOXRESOLVE): LDFLAGS  += $(nix_LDFLAGS) $(sqlite3_LDFLAGS)
lib/$(LIBFLOXRESOLVE): LDFLAGS  += -Wl,--no-as-needed
lib/$(LIBFLOXRESOLVE): $(addprefix src/,resolve.o descriptor.o preferences.o)
lib/$(LIBFLOXRESOLVE): $(addprefix src/,inputs.o walk.o util.o attr-path-glob.o)
lib/$(LIBFLOXRESOLVE): $(addprefix src/,descriptor-functor.o semver.o)
lib/$(LIBFLOXRESOLVE): $(addprefix src/,package.o flox-flake.o resolver-state.o)
	$(CXX) $^ $(LDFLAGS) -o "$@"



# ---------------------------------------------------------------------------- #

bin/%:        CXXFLAGS += $(bin_CXXFLAGS)
bin/resolver: CXXFLAGS += $(sqlite3_CFLAGS) $(nljson_CFLAGS) $(nix_CFLAGS)
bin/%:        LDFLAGS  += $(bin_LDFLAGS)
bin/resolver: LDFLAGS  += $(sqlite3_LDFLAGS) $(nix_LDFLAGS)
bin/resolver: LDFLAGS  += $(floxresolve_LDFLAGS)
bin/resolver: src/main.cc lib/$(LIBFLOXRESOLVE)
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
install-include: $(addprefix $(INCLUDEDIR)/,$(COMMON_HEADERS))


# ---------------------------------------------------------------------------- #

.PHONY: tests check

tests/%: CXXFLAGS += $(sqlite3_CFLAGS) $(nljson_CFLAGS)
tests/%: CXXFLAGS += $(nix_CFLAGS) $(nljson_CFLAGS) $(bin_CXXFLAGS)
tests/%: LDFLAGS  += $(sqlite3_LDFLAGS) $(nix_LDFLAGS)
tests/%: LDFLAGS  += $(floxresolve_LDFLAGS) $(bin_LDFLAGS)
$(TESTS:.cc=): %: $(addprefix include/,$(COMMON_HEADERS)) lib/$(LIBFLOXRESOLVE)
$(TESTS:.cc=): %: %.cc
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
	  echo $(nljson_CFLAGS) $(argparse_CFLAGS);                           \
	}|$(TR) ' ' '\n'|$(SED) 's/-std=/%cpp -std=/' >> "$@";


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
