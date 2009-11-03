%{
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "nameless.h"
#include "nameless/parser.h"
#include "nameless/node.h"
#include "nameless/mm.h"
#include "nameless/function.h"

NLS_GLOBAL nls_node *nls_sys_parse_result;

static int yyerror(char *msg);
%}

%union {
	int yst_token;
	int yst_int;
	nls_string *yst_str;
	nls_node *yst_node;
}

%token<yst_token> tSPACE  tNEWLINE
%token<yst_token> tLPAREN tRPAREN
%token<yst_token> tLBRACE tRBRACE
%token<yst_token> tOP_ADD tOP_SUB
%token<yst_token> tOP_MUL tOP_DIV
%token<yst_token> tOP_MOD
%token<yst_int> tNUMBER
%token<yst_str> tIDENT

%type<yst_node> code exprs
%type<yst_node> expr function

%start code

%%
code	: op_spaces
	{
		nls_sys_parse_result = $$ = NULL;
	}
	| op_spaces exprs op_spaces
	{
		$$ = $2;
		nls_sys_parse_result = nls_grab($$);
	}

exprs	: expr
	{
		$$ = nls_list_new($1);
	}
	| exprs op_spaces expr
	{
		nls_list_add($1, $3); /* TODO: malloc failure */
		$$ = $1;
	}

expr	: tNUMBER
	{
		$$ = nls_int_new($1);
	}
	| tIDENT
	{
		$$ = nls_var_new($1);
	}
	| tLBRACE exprs tRBRACE
	{
		$$ = $2;
	}
	| tLPAREN function spaces exprs tRPAREN
	{
		$$ = nls_application_new($2, $4);
	}

function: tOP_ADD { $$ = nls_function_new(nls_func_add); }
	| tOP_SUB { $$ = nls_function_new(nls_func_sub); }
	| tOP_MUL { $$ = nls_function_new(nls_func_mul); }
	| tOP_DIV { $$ = nls_function_new(nls_func_div); }
	| tOP_MOD { $$ = nls_function_new(nls_func_mod); }

op_spaces: /* empty */
	| op_spaces spaces

spaces	: space
	| spaces space

space	: tSPACE
	| tNEWLINE

%%

static int
yyerror(char *msg)
{
	NLS_ERROR("%s", msg);

	return 0;
}
