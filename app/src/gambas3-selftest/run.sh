#! /bin/sh

TESTSUITE="@Production"

if [ $1 = 'fast' ]
then
	TESTSUITE="@Fast"
fi

MYDIR=$(cd `dirname $0` && pwd)

# run Production without JIT

echo "########### Run without JIT ############"

gbx3 -j -T "$TESTSUITE" $MYDIR

if [ $? -ne 0 ] 
then
	echo Test without JIT failed
	return 1
fi

# run Production with JIT
echo "########### Now run with JIT ############"
GB_JIT_CFLAGS=-O0 gbx3 -T "$TESTSUITE" $MYDIR

if [ $? -ne 0 ] 
then
	echo Test with JIT failed
	return 1
fi


