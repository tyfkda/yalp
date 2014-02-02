YALP=../../yalp -L ../../boot.bin
RUNTEST=$(YALP)

all:	test

test:	boot-test compiler-test setf-test

boot-test:
	$(RUNTEST) boot-test.yl

compiler-test:
	$(RUNTEST) compiler-test.yl

setf-test:
	$(RUNTEST) setf-test.yl
