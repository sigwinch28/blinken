CFLAGS += -I$(UNITY_PATH)/src -I../include -I../

BIN = test_parser

.PHONY: all test clean
.PRECIOUS: $(BIN)

all: test

OBJS = $(BIN).o $(UNITY_PATH)/src/unity.o

test_parser: $(OBJS) ../parser.c ../include/parser.h
	$(LINK.c) $(OBJS) $(LOADLIBES) $(LDLIBS) -o $@

test: $(BIN)
	./$(BIN)

clean:
	-$(RM) $(BIN)
	-$(RM) $(OBJS)

compile_commands.json: FORCE
	+bear --append --use-cc $(CC) $(MAKE) $(BIN)

FORCE: ;
