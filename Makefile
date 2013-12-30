PROJECT=yalp

SRCDIR=src
INCDIR=include
OBJDIR=obj
LIBDIR=lib
LIBNAME=$(LIBDIR)/lib$(PROJECT).a
TOOLSDIR=tools

SRCS=$(wildcard $(SRCDIR)/*.cc)
OBJS=$(subst $(SRCDIR),$(OBJDIR),$(SRCS:%.cc=%.o))
DEPS=$(subst $(SRCDIR),$(OBJDIR),$(SRCS:%.cc=%.d))

CXXFLAGS += -Wall -Wextra -std=c++0x -MMD -I$(INCDIR)  # -Werror

.PHONY: all clean test

all:	$(PROJECT)

clean:
	rm -rf $(OBJDIR) $(LIBDIR)
	rm -f $(PROJECT)
	make -C test clean

-include $(DEPS)
-include $(OBJDIR)/yalp.d

$(PROJECT):	$(OBJS) $(OBJDIR)/yalp.o
	g++ -o $(PROJECT) $(OBJDIR)/yalp.o $(LIBNAME)

$(LIBNAME):	$(OBJS)

$(OBJDIR)/yalp.o:	$(TOOLSDIR)/src/yalp.cc
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/%.o:	$(SRCDIR)/%.cc
	@if ! [ -e $(OBJDIR) ]; then\
	  mkdir $(OBJDIR);\
	fi
	@if ! [ -e $(LIBDIR) ]; then\
	  mkdir $(LIBDIR);\
	fi
	g++ $(CXXFLAGS) -o $@ -c $<
	ar r $(LIBNAME) $@

test:	$(PROJECT)
	make -C compiler test
	make -C test test

check-length:
	wc -l src/* include/**/*.hh compiler/*.yl | sort -nr

boot.bin:	self.bin
	mv self.bin boot.bin

self.bin:	compiler/boot.yl compiler/util.yl compiler/compiler.yl
	./yalp -L boot.bin -C compiler/boot.yl compiler/util.yl compiler/compiler.yl > _self.bin
	./yalp -L boot.bin tools/code-walker.yl tools/optimize.yl < _self.bin > self.bin
	rm _self.bin

self-compile:	self.bin
	diff boot.bin self.bin && rm self.bin && echo OK

include/yalp/binder.inc:	tools/gen_template.rb
	ruby $< > $@
