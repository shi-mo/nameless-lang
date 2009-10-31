%{
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "nameless.h"
#include "nameless/parser.h"
#include "nameless/node.h"
#include "nameless/mm.h"
#include "nameless/function.h"

#define NLS_MSG_BROKEN_LIST        "Broken list"

NLS_GLOBAL nls_node *nls_sys_parse_result;

static int yyerror(char *msg);
static nls_node* nls_int_new(int val);
static nls_node* nls_var_new(nls_string *name);
static nls_node* nls_function_new(nls_function func);
static nls_node* nls_application_new(nls_node *func, nls_node *arg);
static nls_node* nls_list_new(nls_node *node);
static int nls_list_add(nls_node *list, nls_node *node);
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
		nls_sys_parse_result = $$ = $2;
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
	fprintf(stderr, "ERROR: %s\n", msg);

	return 0;
}

/*********************
 * Memory Management *
 *********************/
static nls_node*
nls_int_new(int val)
{
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return NULL;
	}
	node->nn_type = NLS_TYPE_INT;
	node->nn_int = val;
	return node;
}

static nls_node*
nls_var_new(nls_string *name)
{
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return NULL;
	}
	node->nn_type = NLS_TYPE_VAR;
	node->nn_var.nv_name = name;
	return node;
}

static nls_node*
nls_function_new(nls_function func)
{
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return NULL;
	}
	node->nn_type = NLS_TYPE_FUNCTION;
	node->nn_func = func;
	return node;
}

static nls_node*
nls_application_new(nls_node *func, nls_node *arg)
{
	nls_application *app;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return NULL;
	}
	node->nn_type = NLS_TYPE_APPLICATION;
	app = &(node->nn_app);
	app->na_func = func;
	app->na_arg  = arg;
	return node;
}

/*******************
 * List Management *
 *******************/
static nls_node*
nls_list_new(nls_node *item)
{
	nls_list *list;
	nls_node *node = nls_new(nls_node);

	if (!node) {
		return NULL;
	}
	list = &(node->nn_list);
	node->nn_type = NLS_TYPE_LIST;
	list->nl_head = item;
	list->nl_rest = NULL;
	return node;
}

static int
nls_list_add(nls_node *node, nls_node *item)
{
	nls_list *list;
	nls_node *new;

	list = &(node->nn_list);
	if (!list->nl_head) {
		NLS_BUG(NLS_MSG_BROKEN_LIST);
		return 1; /* dummy */
	}
	if (list->nl_rest) {
		return nls_list_add(list->nl_rest, item);
	}

	new = nls_list_new(item);
	if (!new) {
		return 1;
	}
	list->nl_rest = new;
	return 0;
}
