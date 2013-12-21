INCDIR=../include
LIBDIR=../lib
LIBNAME=$(LIBDIR)/libyalp.a

SRCS=$(wildcard *.cc)
DEPS=$(SRCS:%.cc=%.d)
EXES=$(SRCS:%.cc=%)

CXXFLAGS += -Wall -Wextra -Werror -std=c++0x -MMD -I$(INCDIR)

.PHONY: all clean test

all:	$(EXES)

clean:
	rm -rf *.o *.d $(EXES)

-include $(DEPS)

%.o:	%.cc
	g++ $(CXXFLAGS) -o $@ -c $<

001_run_string:	001_run_string.o
	g++ -o $@ $(LIBNAME) 001_run_string.o

002_register_c_func:	002_register_c_func.o
	g++ -o $@ $(LIBNAME) 002_register_c_func.o

003_call_script_func:	003_call_script_func.o
	g++ -o $@ $(LIBNAME) 003_call_script_func.o
