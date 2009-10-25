YACC = yacc -d
CC     = gcc
CFLAGS = -Wall -g
#CFLAGS += -E
#CFLAGS += -DBINOP_DEBUG
#CFLAGS += -DYYDEBUG=1
EXEC = binop

.PHONY: all
all: $(EXEC)

.PHONY: clean
clean:
	rm -f $(EXEC) y.tab.c y.tab.h lex.yy.c

$(EXEC): binop.c y.tab.c lex.yy.c
	$(CC) $(CFLAGS) -o $@ $^

y.tab.c: parse.y binop.h
	$(YACC) $<
	@cp y.tab.h .y.tab.h
	@echo '#include "binop.h"' > y.tab.h
	@cat .y.tab.h >> y.tab.h
	@rm .y.tab.h

y.tab.h: y.tab.c

lex.yy.c: lex.l y.tab.h binop.h
	$(LEX) $<

.PHONY: test
test: $(EXEC)
	@YYDEBUG=1
	@for T in test/*.bop; do \
		echo "==== `basename $$T`"; \
		./$(EXEC) < $$T; \
		if [ 0 -ne $$? ]; then \
			echo "--------"; \
			cat $$T; \
			echo "--------"; \
			break; \
		fi; \
	done
