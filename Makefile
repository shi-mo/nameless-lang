SRCDIR  = src
INCDIR  = include
TESTDIR = test
OBJDIR  = obj

SRCS    = $(wildcard $(SRCDIR)/*.c)
HEADERS = $(wildcard $(INCDIR)/*.h) $(wildcard $(INCDIR)/**/*.h)
OBJS  = $(OBJDIR)/y.tab.o $(OBJDIR)/lex.yy.o
OBJS += $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
EXEC  = nameless

YACC = yacc -d
CC     = gcc
CFLAGS = -Wall -g -I$(INCDIR) -I.
#CFLAGS += -E
#CFLAGS += -DNLS_DEBUG
#CFLAGS += -DYYDEBUG=1

.PHONY: all
all: $(EXEC)

.PHONY: clean
clean:
	rm -rf $(EXEC) $(OBJDIR) lex.yy.c y.tab.c y.tab.h
	find -name '*~' -exec rm {} \;

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/y.tab.o: y.tab.c $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/lex.yy.o: lex.yy.c $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

y.tab.c: $(SRCDIR)/parse.y $(HEADERS)
	$(YACC) $<
	@cp y.tab.h .y.tab.h
	@echo '#include "nameless.h"' > y.tab.h
	@cat .y.tab.h >> y.tab.h
	@rm .y.tab.h

y.tab.h: y.tab.c

lex.yy.c: $(SRCDIR)/lex.l y.tab.h $(HEADERS)
	$(LEX) $<

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
