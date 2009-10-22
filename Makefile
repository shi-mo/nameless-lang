YACC = yacc
CC     = gcc
CFLAGS = -Wall
#CFLAGS += -DDEBUG
#CFLAGS += -DYYDEBUG=1

.PHONY: all
all: binop

.PHONY: clean
clean:
	rm -f binop y.tab.c

y.tab.c: binop.y
	$(YACC) $<

binop:	y.tab.c
	$(CC) $(CFLAGS) -o $@ $^
