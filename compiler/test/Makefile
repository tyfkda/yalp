YALP=../../yalp -L ../../boot.bin
RUNTEST=$(YALP)

all:	test

test:	boot-test util-test compiler-test

boot-test:
	$(RUNTEST) boot-test.yl

util-test:
	$(RUNTEST) util-test.yl

compiler-test:
	$(RUNTEST) compiler-test.yl
