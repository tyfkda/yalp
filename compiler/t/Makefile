YALP=../../yalp -L ../../boot.bin
RUNTEST=$(YALP) test.arc

all:	test

test:	util-test compiler-test
	echo SUCCESS

util-test:
	$(RUNTEST) util-test.arc

compiler-test:
	$(RUNTEST) compiler-test.arc
