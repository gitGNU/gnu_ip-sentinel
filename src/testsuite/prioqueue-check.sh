#! /bin/bash

: ${srcdir=.}
. ${srcdir}/testsuite/functions

outfile_err=`mktemp /tmp/ip-sentinel.check.XXXXXX`
outfile_out=`mktemp /tmp/ip-sentinel.check.XXXXXX`

trap "rm -f ${outfile_err} ${outfile_out}" EXIT

execfile=./prioqueue-check

function execprog()
{
    "$@" ${execfile} $(cat ${basefile}.inp) >${outfile_out}
}

function verify()
{
    sed -e "${REPLACE_REGEX}" ${outfile_out} |
	$DIFF ${basefile}.out - || exit 1
}


file ${execfile} | grep -q 'statically linked' || {
    exists ef       && { execprog ef 2>&1 | sed -e '1,2d'; } && verify
    exists valgrind && execprog valgrind --tool=memcheck -q && verify
}

execprog && verify
