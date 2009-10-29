INCDIR  = include
TESTDIR = test
OBJDIR  = obj

SRCS    = $(wildcard *.c)
HEADERS = $(wildcard $(INCDIR)/*.h) $(wildcard $(INCDIR)/**/*.h)
GENERATED = lex.yy.c y.tab.c y.tab.h
OBJS  = $(OBJDIR)/y.tab.o $(OBJDIR)/lex.yy.o
OBJS += $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
EXEC  = nameless

YACC = yacc -d
CC     = gcc
CFLAGS = -Wall -g -I$(INCDIR) -I.
#CFLAGS += -E
#CFLAGS += -DNLS_DEBUG
#CFLAGS += -DYYDEBUG=1

.PHONY: all
all: $(EXEC)

.PHONY: codegen
codegen: $(GENERATED)

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(GENERATED)
	find -name '*~' -exec rm {} \;

.PHONY: clobber
clobber: clean
	rm -f $(EXEC)

.PHONY: test
test: $(EXEC)
	@YYDEBUG=1
	@for T in $(TESTDIR)/*.nls; do \
		echo "==== `basename $$T`"; \
		./$(EXEC) < $$T; \
		STATUS=$$?; \
		if [ 0 -ne $$STATUS ]; then \
			echo "exit status:$$STATUS"; \
			echo "--------"; \
			cat $$T; \
			echo "--------"; \
			break; \
		fi; \
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

lex.yy.c: lex.l y.tab.h $(HEADERS)
	$(LEX) $<
