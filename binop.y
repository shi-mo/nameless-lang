%{
#include "binop.h"

FILE *binop_out;
%}

%token tOP_ADD tOP_SUB
%token tOP_MUL tOP_DIV
%token tOP_MOD
%token tLPAREN tRPAREN
%token tSPACE  tNEWLINE
%token tNUMBER

%%
code	: /* empty */
	| tNEWLINE
	| exprs tNEWLINE
	{
		$$ = $1;
	}

exprs	: expr
	{
		BINOP_CONSOLE("%d", $1);
		$$ = $1;
	}
	| exprs tNEWLINE expr
	{
		BINOP_CONSOLE("%d", $3);
		$$ = $3;
	}

expr	: tNUMBER
	{
		BINOP_DEBUG_PRINT("tNUMBER: %d", $1);
	}
	| tLPAREN operator spaces expr spaces expr tRPAREN
	{
		BINOP_CONSOLE("(%c %d %d)", $2, $4, $6);

		switch ($2) {
		case '+':
			$$ = $4 + $6;
			break;
		case '-':
			$$ = $4 - $6;
			break;
		case '*':
			$$ = $4 * $6;
			break;
		case '/':
			$$ = $4 / $6;
			break;
		case '%':
			$$ = $4 % $6;
			break;
		default:
			YYABORT;
		}
	}

operator: tOP_ADD { $$ = '+'; }
	| tOP_SUB { $$ = '-'; }
	| tOP_MUL { $$ = '*'; }
	| tOP_DIV { $$ = '/'; }
	| tOP_MOD { $$ = '%'; }

spaces	: tSPACE
	| spaces tSPACE

%%

int
yyerror(char *msg)
{
	fprintf(stderr, "ERROR: %s\n", msg);

	return 0;
}
