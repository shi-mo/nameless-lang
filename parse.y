%{
/*
 * Nameless - A lambda calculation language.
 * Copyright (C) 2009 Yoshifumi Shimono <yoshifumi.shimono@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <ctype.h>
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
%token<yst_token> tDOT    tLAMBDA
%token<yst_int> tNUMBER
%token<yst_str> tIDENT

%type<yst_node> code exprs
%type<yst_node> expr abstraction

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
	| exprs spaces expr
	{
		nls_list_add($1, $3);
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
	| tLAMBDA tLPAREN exprs tRPAREN tDOT expr
	{
		$$ = nls_abstraction_new($3, $6);
	}
	| tLPAREN expr tRPAREN
	{
		$$ = nls_list_new($2);
	}
	| tLPAREN exprs spaces expr tRPAREN
	{
		nls_list_add($2, $4);
		$$ = $2;
	}
	| abstraction tLPAREN op_spaces exprs op_spaces tRPAREN
	{
		$$ = nls_application_new($1, $4);
	}

abstraction: tIDENT
	{
		$$ = nls_var_new($1);
	}
	| tLPAREN expr tRPAREN
	{
		$$ = $2;
	}

op_spaces: /* empty */
	| spaces

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
