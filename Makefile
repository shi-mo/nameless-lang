YACC = yacc
CC     = gcc
CFLAGS = -DDEBUG

.PHONY: all
all: binop

.PHONY: clean
clean:
	rm -f binop y.tab.c

y.tab.c: binop.y
	$(YACC) $<

binop:	y.tab.c
	$(CC) $(CFLAGS) -o $@ $^
