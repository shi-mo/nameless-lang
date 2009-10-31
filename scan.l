%{
#include <stdio.h>
#include "y.tab.h"
#include "nameless.h"

#define NLS_MSG_ILLEGAL_TOKEN "Illegal token"

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
