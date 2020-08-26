# Makefile for building wasm c algorithms

ifneq ($(DEBUG),1)
  CC=emcc
  EFLAGS = -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' --pre-js preamble.js --post-js postamble.js
  CFLAGS = -DDEBUG -g -Wall $(EFLAGS)
  EXECUTABLE = out/rshape.js
else
  CC=gcc
  CFLAGS = -DDEBUG -g -Wall -fsanitize=address
  EXECUTABLE = out/rshape
endif

LDLIBS= -lm



DEPDIR := deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

RM := rm -f

SRCS := utils.c rshape.c ADTlinkedlist.c ADTarraylist.c

#TODO add postamble.js preamble.js pre reqs
$(EXECUTABLE): utils.o rshape.o ADTlinkedlist.o ADTarraylist.o
	$(CC) $(LDLIBS) $(CFLAGS) $^ -o $(EXECUTABLE)

%.o : %.c
%.o: %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(CC) $(DEPFLAGS) $(LDLIBS) $(CFLAGS) -c $<

$(DEPDIR):
	@mkdir -p $@

clean:
	$(RM) *.o *.gch rshape deps/*

# cleans out wasm build and generates local program
debug: clean
	$(MAKE) DEBUG=1

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)

$(DEPFILES):

include $(wildcard $(DEPFILES))
