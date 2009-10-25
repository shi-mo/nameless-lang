%{
#include "nameless.h"

FILE *nls_out;
FILE *nls_err;

static nls_node *nls_int_new(int val);
static nls_node *nls_operator_new(nls_operator op);
static nls_node *nls_list_new(nls_node *node);
static int nls_list_add(nls_node *list, nls_node *node);
static int nls_list_isfull(nls_node *list);
%}

%union {
	int yst_token;
	int yst_int;
	nls_node *yst_node;
}

%token<yst_token> tSPACE  tNEWLINE
%token<yst_token> tLPAREN tRPAREN
%token<yst_token> tLBRACE tRBRACE
%token<yst_token> tOP_ADD tOP_SUB
%token<yst_token> tOP_MUL tOP_DIV
%token<yst_token> tOP_MOD
%token<yst_int> tNUMBER

%type<yst_node> code exprs
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
	| tLBRACE exprs tRBRACE
	{
		$$ = $2;
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
static nls_node*
nls_list_new(nls_node *node)
{
	int sz = (1 << NLS_LIST_ARRAY_EXP);
	nls_node *ls_nd = nls_new(nls_node);
	nls_list *list  = &(ls_nd->nn_union.nnu_list);
	nls_node **ary  = nls_array_new(nls_node*, sz);

	if (ls_nd && ary) {
		ls_nd->nn_type = NLS_TYPE_LIST;
		list->nl_num = 1;
		list->nl_ary_size = sz;
		list->nl_array = ary;
		ary[0] = node;
		return ls_nd;
	}
	if (ary) {
		nls_free(ary);
	}
	if (ls_nd) {
		nls_free(ls_nd);
	}
	return NULL;
}

static int
nls_list_add(nls_node *ls_nd, nls_node *node)
{
	int new_size;
	nls_node **ary;
	nls_list *list = &(ls_nd->nn_union.nnu_list);

	if (!nls_list_isfull(ls_nd)) {
		list->nl_array[list->nl_num++] = node;
		return 0;
	}

	new_size = (list->nl_ary_size << 1);
	ary = nls_array_new(nls_node*, new_size);
	if (ary) {
		list->nl_ary_size = new_size;
		memcpy(ary, list->nl_array,
			(list->nl_ary_size * sizeof(nls_node*)));
		nls_free(list->nl_array);
		list->nl_array = ary;
		list->nl_array[list->nl_num++] = node;
		return 0;
	}
	return 1;
}

static int
nls_list_isfull(nls_node *ls_nd)
{
	nls_list *list = &(ls_nd->nn_union.nnu_list);

	return (list->nl_ary_size == list->nl_num);
}
