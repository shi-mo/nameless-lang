%{
#include <stdio.h>
#include "y.tab.h"
#include "nameless.h"
#include "nameless/mm.h"

#define NLS_MSG_ILLEGAL_TOKEN "Illegal token"

static int yywrap(void);
%}
%%

"("	{ return tLPAREN; }
")"	{ return tRPAREN; }
"."	{ return tDOT; }
[ \t]+	{ return tSPACE; }
\r|\n|\n\r	{ return tNEWLINE; }
"lambda"	{ return tLAMBDA; }

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
