YACC = yacc
CC     = gcc
CFLAGS = -Wall
#CFLAGS += -DDEBUG
#CFLAGS += -DYYDEBUG=1
EXEC = binop

.PHONY: all
all: $(EXEC)

.PHONY: clean
clean:
	rm -f $(EXEC) y.tab.c

y.tab.c: binop.y
	$(YACC) $<

$(EXEC):	y.tab.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: test
test: $(EXEC)
	@for T in test/*; do echo "=== `basename $$T`"; ./$(EXEC) < $$T; done
