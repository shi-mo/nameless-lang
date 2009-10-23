#include "binop.h"

int
main(int argc, char *argv[])
{
	binop_out = stdout;

	return yyparse();
}
