#! /bin/bash

: ${srcdir=.}
. ${srcdir}/testsuite/functions

outfile_err=`mktemp /tmp/ip-sentinel.check.XXXXXX`
outfile_out=`mktemp /tmp/ip-sentinel.check.XXXXXX`
outfile_fd3=`mktemp /tmp/ip-sentinel.check.XXXXXX`
trap "rm -f ${outfile_err} ${outfile_out} ${outfile_fd3}" EXIT

execfile=./blacklist-check
ipfile=${basefile}.ips

test -e ${ipfile} || ipfile=${datadir}/blacklist-check.ips

function execprog()
{
    "$@" ${execfile} $ipfile ${basefile}.{lst,res} >${outfile_out} 3>${outfile_fd3}
}

function verify()
{
    sed -e "${REPLACE_REGEX}" ${outfile_out} |
	$DIFF ${basefile}.out - || exit 1
    sed -e "${REPLACE_REGEX}" ${outfile_fd3} |
	$DIFF ${basefile}.fd3 - || exit 1
}

LANG=C file ${execfile} | grep -q 'statically linked' || {
    exists ef       && { execprog ef 2>&1 | sed -e '1,2d'; } && verify
    exists valgrind && execprog valgrind -q && verify
}

execprog && verify
