INCDIR    = include
TESTDIR   = test
UTDIR     = ut
EXPECTDIR = expect
ACTUALDIR = actual
OBJDIR    = obj

SRCS    = main.c nameless.c mm.c function.c
HEADERS = $(wildcard $(INCDIR)/*.h) $(wildcard $(INCDIR)/**/*.h)
TESTS   = $(wildcard $(TESTDIR)/*.nls)
EXPECTS = $(patsubst $(TESTDIR)/%.nls,$(EXPECTDIR)/%.expect,$(TESTS))
UTSRCS  = lex.yy.c mm.c
UTBINS  = $(patsubst %.c,$(UTDIR)/%.bin,$(UTSRCS))

GENERATED = lex.yy.c y.tab.c y.tab.h
OBJS  = $(OBJDIR)/y.tab.o $(OBJDIR)/lex.yy.o
OBJS += $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
EXEC  = nameless

YACC = yacc -d
CC     = gcc
CFLAGS = -Wall -g -I$(INCDIR) -I.
#CFLAGS += -E
#CFLAGS += -DYYDEBUG=1

.PHONY: all
all: $(EXEC)

.PHONY: codegen
codegen: $(GENERATED)

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(GENERATED) $(ACTUALDIR) $(UTDIR)
	find -name '*~' -exec rm {} \;

.PHONY: clobber
clobber: clean
	rm -f $(EXEC)

.PHONY: test
test: $(EXEC) $(TESTS) $(EXPECTS) $(ACTUALDIR)
	@for T in $(TESTDIR)/*.nls; do \
		NAME=`basename $$T .nls`; \
		echo "==== `basename $$T`"; \
		./$(EXEC) < $$T > $(ACTUALDIR)/$$NAME.actual; \
		STATUS=$$?; \
		if [ 0 -ne $$STATUS ]; then \
			echo "Exit status: $$STATUS"; \
			echo "--------"; \
			cat $$T; \
			echo "--------"; \
			break; \
		fi; \
		diff $(EXPECTDIR)/$$NAME.expect $(ACTUALDIR)/$$NAME.actual; \
		STATUS=$$?; \
		if [ 0 -ne $$STATUS ]; then \
			echo "Test result mismatch."; \
			break; \
		fi; \
	done

.PHONY: unittest
unittest: $(UTBINS)
	@for UT in $^; do \
		echo "==== `basename $$UT .bin`.c"; \
		./$$UT; \
	done

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/y.tab.o: y.tab.c $(HEADERS) $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/lex.yy.o: lex.yy.c $(HEADERS) $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.c $(HEADERS) $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

y.tab.c: parse.y $(HEADERS)
	$(YACC) $<
	@cp y.tab.h .y.tab.h
	@echo '#include "nameless/node.h"' > y.tab.h
	@cat .y.tab.h >> y.tab.h
	@rm .y.tab.h

y.tab.h: y.tab.c

lex.yy.c: scan.l y.tab.h $(HEADERS)
	$(LEX) $<

$(ACTUALDIR):
	mkdir -p $(ACTUALDIR)

$(UTDIR)/%.c: %.c $(UTDIR)
	sh scripts/utgen.sh $< > $@

$(UTDIR):
	mkdir -p $(UTDIR)

$(UTDIR)/%.bin: $(UTDIR)/%.c $(OBJS)
	@DEP=`ls $^ | sed 's/ /\n/g' | grep -v main.o | grep -v \`basename $< .c\`.o | tr '\r\n' ' '`; \
	echo "$(CC) -DNLS_UNIT_TEST $(CFLAGS) -o $@ $$DEP"; \
	$(CC) -DNLS_UNIT_TEST $(CFLAGS) -o $@ $$DEP
