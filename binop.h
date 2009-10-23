#ifndef _BINOP_H_
#define _BINOP_H_

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
extern FILE *binop_out;

/**************
 * PROTOTYPES *
 **************/
int yylex(void);
int yyparse(void);
int yyerror(char *msg);

#endif /* _BINOP_H_ */
