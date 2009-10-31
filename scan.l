%{
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "y.tab.h"
#include "nameless.h"
#include "nameless/mm.h"

#define NLS_MSG_ILLEGAL_TOKEN "Illegal token"
#define NLS_MSG_FAILED_TO_ALLOCATE_MEMORY "Failed to allocate memory"

static nls_string* nls_string_new(size_t len, char *from);
static int yywrap(void);
%}
%%

"+"	{ return tOP_ADD; }
"-"	{ return tOP_SUB; }
"*"	{ return tOP_MUL; }
"/"	{ return tOP_DIV; }
"%"	{ return tOP_MOD; }
"("	{ return tLPAREN; }
")"	{ return tRPAREN; }
"["	{ return tLBRACE; }
"]"	{ return tRBRACE; }
[ \t]+	{ return tSPACE; }
\r|\n|\n\r	{ return tNEWLINE; }

[1-9][0-9]*	{
	int temp;

	sscanf(yytext, "%d", &temp);
	yylval.yst_int = temp;
	return tNUMBER;
}

[_[:alpha:]][_[:alnum:]]*	{
	nls_string *str;
	size_t len = strnlen(yytext, SIZE_MAX-1);

	str = nls_string_new(len, yytext);
	if (!str) {
		NLS_ERROR(NLS_MSG_FAILED_TO_ALLOCATE_MEMORY);
		exit(1);
	}
	yylval.yst_str = str;
	return tIDENT;
}

.	{
	NLS_ERROR(NLS_MSG_ILLEGAL_TOKEN);
	exit(1);
}
%%

static nls_string*
nls_string_new(size_t len, char *from)
{
	char *buf;
	nls_string *str;

	if (!(buf = nls_array_new(char, len+1))) {
		return NULL;
	}
	if (!(str = nls_new(nls_string))) {
		if (buf) {
			nls_free(buf);
		}
		return NULL;
	}
	strncpy(buf, from, len);
	buf[len] = '\0';
	str->ns_len  = len;
	str->ns_bufp = buf;
	return str;
}

static int
yywrap(void)
{
	return 1;
}
