%{
#include "binop.h"

FILE *binop_out;

static int binop_op_add(int, int);
static int binop_op_sub(int, int);
static int binop_op_mul(int, int);
static int binop_op_div(int, int);
static int binop_op_mod(int, int);
%}

%union {
	int yst_token;
	int yst_int;
	binop_node *yst_node;
	binop_node_list *yst_list;
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
		BINOP_DEBUG_PRINT("code=>(empty)");
		binop_parse_result = $$ = NULL;
	}
	| tNEWLINE
	{
		BINOP_DEBUG_PRINT("code=>tNEWLINE");
		binop_parse_result = $$ = NULL;
	}
	| exprs tNEWLINE
	{
		BINOP_DEBUG_PRINT("code=>exprs tNEWLINE: %p", $1);
		binop_parse_result = $$ = $1;
	}

exprs	: expr
	{
		BINOP_DEBUG_PRINT("exprs=>expr");
		$$ = BINOP_NODE_LIST_NEW($1);
	}
	| exprs tNEWLINE expr
	{
		BINOP_DEBUG_PRINT("exprs=>exprs tNEWLINE expr");
		$$ = BINOP_NODE_LIST_ADD($1, $3);
	}

expr	: tNUMBER
	{
		BINOP_DEBUG_PRINT("expr=>tNUMBER: %d", $1);
		$$ = BINOP_INT_NEW($1);
	}
	| tLPAREN operator tSPACE expr tSPACE expr tRPAREN
	{
		BINOP_DEBUG_PRINT("expr=>(operator expr expr)");
		$$ = BINOP_APPLICATION_NEW($2, $4, $6);
	}

operator: tOP_ADD { $$ = BINOP_OPERATOR_NEW(binop_op_add); }
	| tOP_SUB { $$ = BINOP_OPERATOR_NEW(binop_op_sub); }
	| tOP_MUL { $$ = BINOP_OPERATOR_NEW(binop_op_mul); }
	| tOP_DIV { $$ = BINOP_OPERATOR_NEW(binop_op_div); }
	| tOP_MOD { $$ = BINOP_OPERATOR_NEW(binop_op_mod); }

%%

int
yyerror(char *msg)
{
	fprintf(stderr, "ERROR: %s\n", msg);

	return 0;
}

static int
binop_op_add(int a, int b)
{
	return a + b;
}

static int
binop_op_sub(int a, int b)
{
	return a - b;
}

static int
binop_op_mul(int a, int b)
{
	return a * b;
}

static int
binop_op_div(int a, int b)
{
	return a / b;
}

static int
binop_op_mod(int a, int b)
{
	return a % b;
}
