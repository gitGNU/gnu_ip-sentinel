#! /bin/bash

: ${srcdir=.}
. ${srcdir}/testsuite/functions

outfile_err=`mktemp /tmp/ip-sentinel.check.XXXXXX`
outfile_out=`mktemp /tmp/ip-sentinel.check.XXXXXX`

trap "rm -f ${outfile_err} ${outfile_out}" EXIT

execfile=./simulate

function execprog()
{
     "$@" ${execfile} -i ${srcdir}/testsuite/data/simulate.cfg \
	$(cat ${basefile}.cmd) eth0 \
	<${srcdir}/testsuite/data/simulate.inp 10>${outfile_out} 1>&10
}

function verify()
{
    sed -e "${REPLACE_REGEX}" ${outfile_out} |
	diff -b -c - ${basefile}.out || exit 1
}


file ${execfile} | grep -q 'statically linked' || {
    exists ef       && { execprog ef 2>&1 | sed -e '1,2d'; } && verify
    exists valgrind && execprog valgrind -q --logfile-fd=10  && verify
}

execprog && verify
