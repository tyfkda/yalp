YALP=../../yalp
RUNTEST=$(YALP)

all:	test

test:	boot-test setf-test node-test compiler-test

boot-test:
	$(RUNTEST) boot-test.yl

setf-test:
	$(RUNTEST) setf-test.yl

node-test:
	$(RUNTEST) node-test.yl

compiler-test:
	$(RUNTEST) compiler-test.yl
