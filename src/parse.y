%{
#include "nameless.h"

FILE *nls_out;

static nls_node *nls_int_new(int val);
static nls_node *nls_operator_new(nls_operator op);
static nls_node_list* nls_node_list_new(nls_node *node);
%}

%union {
	int yst_token;
	int yst_int;
	nls_node *yst_node;
	nls_node_list *yst_list;
}

%token<yst_token> tSPACE  tNEWLINE
%token<yst_token> tLPAREN tRPAREN
%token<yst_token> tOP_ADD tOP_SUB
%token<yst_token> tOP_MUL tOP_DIV
%token<yst_token> tOP_MOD
%token<yst_int> tNUMBER

%type<yst_list> code exprs
%type<yst_node> expr operator

%start code

%%
code	: op_spaces
	{
		nls_parse_result = $$ = NULL;
	}
	| op_spaces exprs op_spaces
	{
		nls_parse_result = $$ = $2;
	}

exprs	: expr
	{
		$$ = nls_node_list_new($1);
	}
	| exprs op_spaces expr
	{
		nls_node_list_add($1, $3); /* TODO: malloc failure */
		$$ = $1;
	}

expr	: tNUMBER
	{
		$$ = nls_int_new($1);
	}
	| tLPAREN operator spaces expr spaces expr tRPAREN
	{
		$$ = nls_application_new($2, $4, $6);
	}

operator: tOP_ADD { $$ = nls_operator_new(nls_op_add); }
	| tOP_SUB { $$ = nls_operator_new(nls_op_sub); }
	| tOP_MUL { $$ = nls_operator_new(nls_op_mul); }
	| tOP_DIV { $$ = nls_operator_new(nls_op_div); }
	| tOP_MOD { $$ = nls_operator_new(nls_op_mod); }

op_spaces: /* empty */
	| op_spaces spaces

spaces	: space
	| spaces space

space	: tSPACE
	| tNEWLINE

%%

int
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

	if (node) {
		node->nn_type = NLS_TYPE_INT;
		node->nn_union.nnu_int = val;
	}
	return node;
}

static nls_node*
nls_operator_new(nls_operator op)
{
	nls_node *node = nls_new(nls_node);

	if (node) {
		node->nn_type = NLS_TYPE_OPERATOR;
		node->nn_union.nnu_op = op;
	}
	return node;
}

static nls_node*
nls_application_new(nls_node *op, nls_node *left, nls_node *right)
{
	nls_node *node = nls_new(nls_node);

	if (node) {
		nls_application *app;

		node->nn_type = NLS_TYPE_APPLICATION;
		app = &(node->nn_union.nnu_app);
		app->na_op    = op;
		app->na_left  = left;
		app->na_right = right;
	}
	return node;
}

/*******************
 * List Management *
 *******************/
static nls_node_list*
nls_node_list_new(nls_node *node)
{
	nls_node_list *list = nls_new(nls_node_list);

	if (list) {
		list->nnl_node = node;
		list->nnl_prev = list;
		list->nnl_next = NULL;
	}
	return list;
}

static int
nls_node_list_add(nls_node_list *list, nls_node *node)
{
	nls_node_list *new_list = nls_node_list_new(node);

	if (new_list) {
		list->nnl_prev->nnl_next = new_list;
		new_list->nnl_prev = list->nnl_prev;
		list->nnl_prev = new_list;
		return 0;
	}
	return 1;
}
