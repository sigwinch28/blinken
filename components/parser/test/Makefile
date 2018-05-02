CFLAGS += -I$(UNITY_PATH)/src -I../include

BIN = test_parser

.PHONY: all test clean
.PRECIOUS: $(BIN)

all: test

OBJS = $(BIN).o $(UNITY_PATH)/src/unity.o ../include/parser.o

test_parser: $(OBJS)
#	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

test: $(BIN)
	./$(BIN)

clean:
	-$(RM) $(BIN)
	-$(RM) $(OBJS)

compile_commands.json: FORCE
	+bear --append --use-cc $(CC) $(MAKE) $(BIN)

FORCE: ;
