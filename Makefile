YACC = yacc -d
CC     = gcc
CFLAGS = -Wall
#CFLAGS += -DDEBUG
#CFLAGS += -DYYDEBUG=1
EXEC = binop

.PHONY: all
all: $(EXEC)

.PHONY: clean
clean:
	rm -f $(EXEC) y.tab.c y.tab.h lex.yy.c

$(EXEC): main.c y.tab.c lex.yy.c
	$(CC) $(CFLAGS) -o $@ $^

y.tab.c: binop.y
	$(YACC) $<

y.tab.h: y.tab.c

lex.yy.c: binop.l y.tab.h
	$(LEX) $<

.PHONY: test
test: $(EXEC)
	@for T in test/*; do echo "=== `basename $$T`"; ./$(EXEC) < $$T; done
