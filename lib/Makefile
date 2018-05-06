CFLAGS += -I./include -fPIC

OBJS = bproto.o

SHARED = libbproto.so
STATIC = libbproto.a

.PHONY: all clean

all: $(SHARED) $(STATIC)

$(SHARED): $(OBJS)
	$(LINK.c) -shared $^ $(LDLIBS) -o $@

$(STATIC): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

clean:
	-$(RM) $(SHARED) $(STATIC)
	-$(RM) $(OBJS)

compile_commands.json: FORCE
	+bear --append --use-cc $(CC) $(MAKE) all

FORCE: ;
