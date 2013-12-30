INCDIR=../include
LIBDIR=../lib
LIBNAME=$(LIBDIR)/libyalp.a

SRCS=$(wildcard *.cc)
DEPS=$(SRCS:%.cc=%.d)
EXES=$(SRCS:%.cc=%)

CXXFLAGS += -Wall -Wextra -std=c++0x -MMD -I$(INCDIR)  # -Werror

.PHONY: all clean test

all:	$(EXES)

clean:
	rm -rf *.o *.d $(EXES)

-include $(DEPS)

%.o:	%.cc
	g++ $(CXXFLAGS) -o $@ -c $<

001_run_string:	001_run_string.o
	g++ -o $@ 001_run_string.o $(LIBNAME)

002_register_c_func:	002_register_c_func.o
	g++ -o $@ 002_register_c_func.o $(LIBNAME)

003_call_script_func:	003_call_script_func.o
	g++ -o $@ 003_call_script_func.o $(LIBNAME)

004_use_binder:	004_use_binder.o
	g++ -o $@ 004_use_binder.o $(LIBNAME)
