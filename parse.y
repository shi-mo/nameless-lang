%{
#include <ctype.h>
#include <stdio.h>
#include "nameless.h"
#include "nameless/parser.h"
#include "nameless/node.h"
#include "nameless/mm.h"
#include "nameless/function.h"

#define NLS_MSG_NO_SUCH_SYMBOL "No such symbol"

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
%token<yst_int> tNUMBER
%token<yst_str> tIDENT

%type<yst_node> code exprs
%type<yst_node> expr

%start code

%%
code	: op_spaces
	{
		nls_sys_parse_result = $$ = NULL;
	}
	| op_spaces exprs op_spaces
	{
		$$ = $2;
		nls_sys_parse_result = nls_node_grab($$);
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

expr	: expr tLPAREN op_spaces exprs op_spaces tRPAREN
	{
		$$ = nls_application_new($1, $4);
	}
	| tNUMBER
	{
		$$ = nls_int_new($1);
	}
	| tIDENT
	{
		nls_hash_entry *ent, *prev;

		ent = nls_hash_search(&nls_sys_sym_table, $1, &prev);
		if (!ent) {
			$$ = nls_var_new($1);
		} else {
			nls_string_free($1);
			$$ = ent->nhe_node;
		}
	}
	| tLPAREN exprs tRPAREN
	{
		$$ = $2;
	}

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
