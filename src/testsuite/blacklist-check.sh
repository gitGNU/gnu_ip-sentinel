#! /bin/bash

datadir=${srcdir}/testsuite/data

outfile_err=`mktemp /tmp/ip-sentinel.check.XXXXXX`
outfile_out=`mktemp /tmp/ip-sentinel.check.XXXXXX`
outfile_fd3=`mktemp /tmp/ip-sentinel.check.XXXXXX`
trap "rm -f ${outfile_err} ${outfile_out} ${outfile_fd3}" EXIT


execfile=./blacklist-check

basefile=`basename $0`
basefile=${datadir}/${basefile%%.sh}
ipfile=${basefile}.ips
test -e ${ipfile} || ipfile=${datadir}/blacklist-check.ips


export EF_PROTECT_BELOW=1
export EF_PROTECT_FREE=1
export EF_FILL=42

function exists()
{
    which "$1" >/dev/null 2>&1
}

function execprog()
{
    "$@" ${execfile} $ipfile ${basefile}.{lst,res} >${outfile_out} 3>${outfile_fd3}
}

function verify()
{
    sed -e 's!^.*\(: (Re)reading blacklist\)!TIME\1!;
	    s!\(de:ad:be:ef:0:\)[^[:blank:]]*!\1XX!g' ${outfile_out} |
	diff -c - ${basefile}.out || exit 1
    diff -c ${outfile_fd3} ${basefile}.fd3 || exit 1
}

set -e


file ${execfile} | grep -q 'statically linked' || {
    exists ef       && { execprog ef 2>&1 | sed -e '1,2d'; } && verify
    exists valgrind && execprog valgrind -q && verify
}

execprog && verify

