YALP=../../yalp -L ../../boot.bin
RUNTEST=$(YALP) test.arc

all:	test

test:	boot-test util-test compiler-test

boot-test:
	$(RUNTEST) boot-test.arc

util-test:
	$(RUNTEST) util-test.arc

compiler-test:
	$(RUNTEST) compiler-test.arc
