YACC = yacc -d
CC     = gcc
CFLAGS = -Wall -g
#CFLAGS += -E
#CFLAGS += -DBINOP_DEBUG
#CFLAGS += -DYYDEBUG=1
EXEC = nameless

.PHONY: all
all: $(EXEC)

.PHONY: clean
clean:
	rm -f $(EXEC) y.tab.c y.tab.h lex.yy.c

$(EXEC): nameless.c y.tab.c lex.yy.c
	$(CC) $(CFLAGS) -o $@ $^

y.tab.c: parse.y nameless.h
	$(YACC) $<
	@cp y.tab.h .y.tab.h
	@echo '#include "nameless.h"' > y.tab.h
	@cat .y.tab.h >> y.tab.h
	@rm .y.tab.h

y.tab.h: y.tab.c

lex.yy.c: lex.l y.tab.h nameless.h
	$(LEX) $<

.PHONY: test
test: $(EXEC)
	@YYDEBUG=1
	@for T in test/*.nls; do \
		echo "==== `basename $$T`"; \
		./$(EXEC) < $$T; \
		if [ 0 -ne $$? ]; then \
			echo "--------"; \
			cat $$T; \
			echo "--------"; \
			break; \
		fi; \
	done
