%{
#include <stdio.h>
#include <ctype.h>

/**********
 * MACROS *
 **********/
#ifdef DEBUG
# define BINOP_DEBUG_PRINT(fmt, ...) \
	fprintf(stderr, "Debug:%s:%d:" fmt "\n", \
	__FILE__, __LINE__,  __VA_ARGS__)
#else
# define BINOP_DEBUG_PRINT(fmt, ...) \
	do {} while(0)
#endif

#define BINOP_CONSOLE(fmt, ...) \
	fprintf(binop_out, "> " fmt "\n", __VA_ARGS__)

/********************
 * GLOBAL VARIABLES *
 ********************/
static FILE *binop_out;

/**************
 * PROTOTYPES *
 **************/
int main(int argc, char *argv[]);
static int yylex(void);
static int yyerror(char *msg);
%}

%token TKN_NUMBER

%%
code	: op_separators exprs op_separators

exprs	: expr
	{
		BINOP_CONSOLE("%d", $1);
	}
	| exprs separators expr
	{
		BINOP_CONSOLE("%d", $3);
	}

expr	: TKN_NUMBER
	{
		BINOP_DEBUG_PRINT("TKN_NUMBER: %d", $1);
	}
	| '(' op ' ' expr ' ' expr ')'
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
			YYERROR;
		}
	}

op	: '+' { $$ = '+'; }
	| '-' { $$ = '-'; }
	| '*' { $$ = '*'; }
	| '/' { $$ = '/'; }
	| '%' { $$ = '%'; }

separators	: separator op_separators

op_separators	: /* empty */
		| separator op_separators

separator	: '\n'
		| '\r'
		| '\t'
		| ' '
%%

int
main(int argc, char *argv[])
{
	binop_out = stdout;

	return yyparse();
}

#define BINOP_LEX_BUF_SIZE 128
static int
yylex(void)
{
	FILE *in = stdin;
	int c = getc(in);

	/* discard \n */
	if (!isdigit(c)) {
		return c;
	}
	{
		char buf[BINOP_LEX_BUF_SIZE];
		char *bufp = buf;

		*(bufp++) = c;
		while (isdigit(c = getc(in))) {
			*(bufp++) = c;
			if (bufp >= (buf + BINOP_LEX_BUF_SIZE - 1)) {
				yyerror("number token too long.");
			}
		}
		ungetc(c, in);
		*bufp = '\0';
		yylval = atoi(buf);
		return TKN_NUMBER;
	}
}

static int
yyerror(char *msg)
{
	fprintf(stderr, "ERROR: %s\n", msg);

	return 0;
}
