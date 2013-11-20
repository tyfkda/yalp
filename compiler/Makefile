all:
	echo 'Use "make test"'
	exit 1

test:	util-test compiler-test vm-test
	echo SUCCESS

util-test:
	gosh util-test.scm

compiler-test:
	gosh compiler-test.scm

vm-test:
	./test.sh
