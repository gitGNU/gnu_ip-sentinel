#! /bin/bash

: ${srcdir=.}
. ${srcdir}/testsuite/functions

outfile_err=`mktemp /tmp/ip-sentinel.check.XXXXXX`
outfile_out=`mktemp /tmp/ip-sentinel.check.XXXXXX`

trap "rm -f ${outfile_err} ${outfile_out}" EXIT

execfile=./simulate

function execprog()
{
    local ext=$(basename "$basefile")
    local cfg=
    local inp=
    ext=${ext##simulate-}

    case "$ext" in
	L-*|R-*) cfg=${srcdir}/testsuite/data/simulate.cfg
		 inp=${srcdir}/testsuite/data/simulate.inp
		 ;;
	*)	 cfg=${basefile}.cfg
		 inp=${basefile}.inp
		 ;;
    esac

    "$@" ${execfile} -i ${cfg} $(cat ${basefile}.cmd) eth0 \
    <${inp} 10>${outfile_out} 1>&10
}

function verify()
{
    sed -e "${REPLACE_REGEX}" ${outfile_out} |
	$DIFF ${basefile}.out - || exit 1
}


file ${execfile} | grep -q 'statically linked' || {
    exists ef       && { execprog ef 2>&1 | sed -e '1,2d'; } && verify
    exists valgrind && execprog valgrind -q --logfile-fd=10  && verify
}

execprog && verify
