%{
#include <stdio.h>

#ifdef DEBUG
# define BINOP_DEBUG_PRINT(fmt, ...) \
	printf("Debug:%d:" fmt, __LINE__,  __VA_ARGS__)
#else
# define BINOP_DEBUG_PRINT(fmt, ...) \
	do {} while(0)
#endif

int main(int argc, char *argv[]);
static int yylex(void);
static int yyerror(char *msg);
%}
%token NUMBER
%token OP_ADD OP_SUB OP_MUL OP_DIV OP_MOD

%%
expr	: NUMBER
	{
		BINOP_DEBUG_PRINT("val:%5d\n", $1);
	}
	| '(' op ' ' expr ' ' expr ')'
	{
		BINOP_DEBUG_PRINT("op:%d val1:%5d val2:%5d\n", $2, $4, $6);
		switch ($2) {
		case OP_ADD:
			$$ = $4 + $6;
		case OP_SUB:
			$$ = $4 - $6;
		case OP_MUL:
			$$ = $4 * $6;
		case OP_DIV:
			$$ = $4 / $6;
		case OP_MOD:
			$$ = $4 % $6;
		}
	}

op	: '+' { $$ = OP_ADD; }
	| '-' { $$ = OP_SUB; }
	| '*' { $$ = OP_MUL; }
	| '/' { $$ = OP_DIV; }
	| '%' { $$ = OP_MOD; }
%%

int
main(int argc, char *argv[])
{
	return yyparse();
}

#define BINOP_LEX_BUF_SIZE 128
static int
yylex(void)
{
	FILE *in = stdin;
	int c = getc(in);

	/* discard \n */
	while ('\n' == c) {
		c = getc(in);
	}
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
		return NUMBER;
	}
}

static int
yyerror(char *msg)
{
	fprintf(stderr, "ERROR: %s\n", msg);

	return 0;
}
