#ifndef _NAMELESS_PARSER_H_
#define _NAMELESS_PARSER_H_

#include <stdio.h>
#include "nameless/node.h"

extern FILE *yyin;
extern FILE *yyout;
extern nls_node *nls_parse_result;

int yylex(void);
int yyparse(void);

#endif /* _NAMELESS_PARSER_H_ */
