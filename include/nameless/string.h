#ifndef _NAMELESS_STRING_H_
#define _NAMELESS_STRING_H_

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

typedef struct _nls_string {
	int ns_len;
	int ns_hash;
	char *ns_bufp;
} nls_string;

nls_string* nls_string_new(char *s);
void nls_string_free(void *ptr);
int nls_strcmp(nls_string *s1, nls_string *s2);

#endif /* _NAMELESS_STRING_H_ */
