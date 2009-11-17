#
# Nameless - A lambda calculation language.
# Copyright (C) 2009 Yoshifumi Shimono <yoshifumi.shimono@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

INCDIR    = include
TESTDIR   = test
UTDIR     = ut
EXPECTDIR = expect
ACTUALDIR = actual
OBJDIR    = obj
DOCDIR    = doc

SRCS     = main.c nameless.c mm.c node.c hash.c string.c function.c
HEADERS  = $(wildcard $(INCDIR)/*.h) $(wildcard $(INCDIR)/**/*.h)
TESTS    = $(wildcard $(TESTDIR)/*.nls)
EXPECTS  = $(patsubst $(TESTDIR)/%.nls,$(EXPECTDIR)/%.expect,$(TESTS))
UTSRCS   = mm.c node.c hash.c string.c
UTBINS   = $(patsubst %.c,$(UTDIR)/%.bin,$(UTSRCS))
DOXYFILE = Doxyfile

GENERATED = lex.yy.c y.tab.c y.tab.h
OBJS  = $(OBJDIR)/y.tab.o $(OBJDIR)/lex.yy.o
OBJS += $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
EXEC  = nameless

YACC   = yacc -d
CC     = gcc
CFLAGS = -Wall -g -I$(INCDIR) -I.
#CFLAGS += -E
#CFLAGS += -DYYDEBUG=1

.PRECIOUS: $(patsubst %.c,$(UTDIR)/%.c,$(UTSRCS))

.PHONY: all
all: $(EXEC)

.PHONY: codegen
codegen: $(GENERATED)

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(GENERATED) $(ACTUALDIR) $(UTDIR) $(DOCDIR)
	find -name '*~' -exec rm {} \;

.PHONY: clobber
clobber: clean
	rm -f $(EXEC)

.PHONY: testall
testall: unittest test

.PHONY: test
test: $(EXEC) $(TESTS) $(EXPECTS) $(ACTUALDIR)
	@for T in $(TESTDIR)/*.nls; do \
		NAME=`basename $$T .nls`; \
		echo "==== `basename $$T`"; \
		./$(EXEC) < $$T | tr -d '\r' > $(ACTUALDIR)/$$NAME.actual; \
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
		STATUS=$$?; \
		if [ 0 -ne $$STATUS ]; then \
			echo "Exit status: $$STATUS"; \
			break; \
		fi; \
	done

$(DOCDIR): $(SRCS) $(HEADERS)
	doxygen $(DOXYFILE)

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

tags: clean
	ctags -R

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
