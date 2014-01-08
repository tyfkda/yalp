YALP=../../yalp -L ../../boot.bin
RUNTEST=$(YALP)

all:	test

test:	boot-test util-test traverse-test

boot-test:
	$(RUNTEST) boot-test.yl

util-test:
	$(RUNTEST) util-test.yl

traverse-test:
	$(RUNTEST) traverse-test.yl
