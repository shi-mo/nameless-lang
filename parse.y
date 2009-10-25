%{
#include "nameless.h"

FILE *nls_out;

static int nls_op_add(int, int);
static int nls_op_sub(int, int);
static int nls_op_mul(int, int);
static int nls_op_div(int, int);
static int nls_op_mod(int, int);
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
code	: /* empty */
	{
		NLS_DEBUG_PRINT("code=>(empty)");
		nls_parse_result = $$ = NULL;
	}
	| tNEWLINE
	{
		NLS_DEBUG_PRINT("code=>tNEWLINE");
		nls_parse_result = $$ = NULL;
	}
	| exprs tNEWLINE
	{
		NLS_DEBUG_PRINT("code=>exprs tNEWLINE: %p", $1);
		nls_parse_result = $$ = $1;
	}

exprs	: expr
	{
		NLS_DEBUG_PRINT("exprs=>expr");
		$$ = NLS_NODE_LIST_NEW($1);
	}
	| exprs tNEWLINE expr
	{
		NLS_DEBUG_PRINT("exprs=>exprs tNEWLINE expr");
		$$ = NLS_NODE_LIST_ADD($1, $3);
	}

expr	: tNUMBER
	{
		NLS_DEBUG_PRINT("expr=>tNUMBER: %d", $1);
		$$ = NLS_INT_NEW($1);
	}
	| tLPAREN operator tSPACE expr tSPACE expr tRPAREN
	{
		NLS_DEBUG_PRINT("expr=>(operator expr expr)");
		$$ = NLS_APPLICATION_NEW($2, $4, $6);
	}

operator: tOP_ADD { $$ = NLS_OPERATOR_NEW(nls_op_add); }
	| tOP_SUB { $$ = NLS_OPERATOR_NEW(nls_op_sub); }
	| tOP_MUL { $$ = NLS_OPERATOR_NEW(nls_op_mul); }
	| tOP_DIV { $$ = NLS_OPERATOR_NEW(nls_op_div); }
	| tOP_MOD { $$ = NLS_OPERATOR_NEW(nls_op_mod); }

%%

int
yyerror(char *msg)
{
	fprintf(stderr, "ERROR: %s\n", msg);

	return 0;
}

static int
nls_op_add(int a, int b)
{
	return a + b;
}

static int
nls_op_sub(int a, int b)
{
	return a - b;
}

static int
nls_op_mul(int a, int b)
{
	return a * b;
}

static int
nls_op_div(int a, int b)
{
	return a / b;
}

static int
nls_op_mod(int a, int b)
{
	return a % b;
}
