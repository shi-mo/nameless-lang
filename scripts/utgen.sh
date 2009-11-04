#!/bin/sh

if [ 1 -ne $# ]; then
	echo "usage: `basename $0` src.c" 1>&2
	exit 1
fi

SRC=$1

cat ${SRC}
cat << EOL
#ifdef NLS_UNIT_TEST
int
main()
{
	nls_init();

EOL

grep '^test_' ${SRC} | sed 's/^\(test_[_[:alnum:]]*\)(.*$/\t\1();/'

cat << EOL

        fprintf(stdout, "\n");
	nls_term();
	return 0;
}
#endif /* NLS_UNIT_TEST */
EOL
