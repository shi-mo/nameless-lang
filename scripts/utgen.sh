#!/bin/sh

#
# Nameless - A lambda calculation language.
# Copyright (C) 2009 Yoshifumi Shimono <yoshifumi.shimono@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

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
	nls_init(stdout, stderr);

EOL

grep '^test_' ${SRC} | sed 's/^\(test_[_[:alnum:]]*\)(.*$/	\1();/'

cat << EOL

        fprintf(stdout, "\n");
	nls_term();
	return 0;
}
#endif /* NLS_UNIT_TEST */
EOL
