#ifndef _NAMELESS_STRING_H_
#define _NAMELESS_STRING_H_

typedef struct _nls_string {
	int ns_len;
	int ns_hash;
	char *ns_bufp;
} nls_string;

nls_string* nls_string_new(char *s);
nls_string* nls_string_grab(nls_string *str);
void nls_string_release(nls_string *str);
void nls_string_free(nls_string *str);
int nls_strcmp(nls_string *s1, nls_string *s2);

#endif /* _NAMELESS_STRING_H_ */
