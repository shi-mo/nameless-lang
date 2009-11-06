%{
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include "y.tab.h"
#include "nameless.h"
#include "nameless/mm.h"

#define NLS_MSG_ILLEGAL_TOKEN "Illegal token"
#define NLS_MSG_FAILED_TO_ALLOCATE_MEMORY "Failed to allocate memory"

static int yywrap(void);
%}
%%

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

[_+\-*/%[:alpha:]][_[:alnum:]]*	{
	nls_string *str;

	str = nls_string_new(yytext);
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

static int
yywrap(void)
{
	return 1;
}
