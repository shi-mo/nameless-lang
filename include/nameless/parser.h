#ifndef _NAMELESS_PARSER_H_
#define _NAMELESS_PARSER_H_

/*
 * Nameless - A lambda calculation language.
 * Copyright (C) 2009 Yoshifumi Shimono <yoshifumi.shimono@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include "nameless/node.h"

extern FILE *yyin;
extern FILE *yyout;
extern int yydebug;
extern nls_node *nls_sys_parse_result;

int yylex(void);
int yyparse(void);

#endif /* _NAMELESS_PARSER_H_ */
