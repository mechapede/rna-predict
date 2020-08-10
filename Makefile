# Makefile for building wasm c algorithms
CC=emcc
EFLAGS = -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' --pre-js preamble.js --post-js postamble.js
CFLAGS = -DNDEBUG -g -Wall $(EFLAGS)
LDLIBS= -lm
DEPDIR := deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

RM := rm -f

SRCS := utils.c rshape.c ADTlinkedlist.c ADTarraylist.c

#TODO add postamble.js preamble.js pre reqs
rshape.js: utils.o rshape.o ADTlinkedlist.o ADTarraylist.o
	$(CC) $(LDLIBS) $(CFLAGS) $^ -o out/rshape.js

%.o : %.c
%.o: %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(CC) $(DEPFLAGS) $(LDLIBS) $(CFLAGS) -c $<

$(DEPDIR):
	@mkdir -p $@

clean:
	$(RM) *.o *.gch rshape deps/*

debug:
	$(MAKE) CFLAGS='-Wextra -pedantic-errors -fsanitize=address -Wall -g -DDEBUG'

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)

$(DEPFILES):

include $(wildcard $(DEPFILES))
