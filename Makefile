# Globals
CFLAGS += -O0 -I$(LIBDIR)/include
LDFLAGS += -L$(BUILDDIR) -I$(LIBDIR)/include

BUILDDIR = ./build

BIN.c = $(LINK.c) $(OBJS) $(LOADLIBES) $(LDLIBS)

# Library
LIBDIR = ./lib
SHAREDLIB = $(BUILDDIR)/libbproto.so
STATICLIB = $(BUILDDIR)/libbproto.a
LIBBUILDDIR = $(BUILDDIR)/lib
LIBOBJS = $(addprefix $(LIBBUILDDIR)/, bproto.o)

# Tests
TESTDIR = ./test
TESTBIN = $(TESTBUILDDIR)/bproto
TESTBUILDDIR= $(BUILDDIR)/test
TESTOBJS = $(addprefix $(TESTBUILDDIR)/, test_bproto.o)

CK_VERBOSITY ?= verbose

VALGRIND ?= valgrind
VALGRINDFLAGS += --leak-check=full

# ESP
ESPBUILDDIR = ./build/esp
ESPDIR = ./esp

# Python
PYTHONBUILDDIR = $(BUILDDIR)/python
PYTHONDIR = ./python
PYTHON ?= python3
PYTHONSETUP ?= setup.py
PYTHONDIST = ./sdist

################################################################################
# Globals
################################################################################

.PHONY: all lib test check python esp clean

all: lib python esp

check: test

clean:
	-$(RM) -rf $(BUILDDIR)

compile_commands.json: FORCE
	+bear --append --use-cc $(CC) $(MAKE) all

FORCE: ;

################################################################################
# Library
################################################################################
$(LIBBUILDDIR):
	mkdir -p $@

$(LIBOBJS): $(LIBBUILDDIR)/%.o: $(LIBDIR)/%.c | $(LIBBUILDDIR)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(SHAREDLIB): LDLIBS=
$(SHAREDLIB): LDFLAGS=
$(SHAREDLIB): CFLAGS += -fPIC -shared
$(SHAREDLIB): $(LIBOBJS)
	$(LINK.c) $^ $(LDLIBS) -o $@

$(STATICLIB): LDLIBS=
$(STATICLIB): $(LIBOBJS) | $(LIBBUILDDIR)
	$(AR) $(ARFLAGS) $@ $(LIBOBJS)

lib: $(SHAREDLIB) $(STATICLIB)

################################################################################
# Tests
################################################################################
$(TESTBUILDDIR):
	mkdir -p $@

$(TESTBIN): OBJS=$(TESTOBJS)

$(TESTBIN): CFLAGS += $(shell pkg-config --cflags check)
$(TESTBIN): LDLIBS += -lbproto $(shell pkg-config --libs check)
$(TESTBIN): $(TESTOBJS) $(SHAREDLIB)
	$(BIN.c) -o $@

$(TESTOBJS): $(TESTBUILDDIR)/%.o: $(TESTDIR)/%.c | $(TESTBUILDDIR)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

test: VALGRINDFLAGS += --quiet
test: $(TESTBIN)
	export LD_LIBRARY_PATH=$(BUILDDIR); \
	export CK_VERBOSITY=$(CK_VERBOSITY); \
	$(VALGRIND) $(VALGRINDFLAGS) $(TESTBIN)


################################################################################
# ESP
################################################################################

esp:
	export BUILD_DIR_BASE=../$(ESPBUILDDIR) ;\
	$(MAKE) -C $(ESPDIR) -s all

################################################################################
# Python
################################################################################
$(PYTHONBUILDDIR):
	mkdir -p $@

python:
	$(PYTHON) $(PYTHONSETUP) sdist --dist-dir $(PYTHONBUILDDIR)
