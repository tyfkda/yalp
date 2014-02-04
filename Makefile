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

CXXFLAGS += -Wall -Wextra -std=c++0x -MMD -I$(INCDIR) -O2  # -Werror

.PHONY: all clean test

all:	$(PROJECT)

clean:
	rm -rf $(OBJDIR) $(LIBDIR)
	rm -f $(PROJECT)
	make -C test clean
	make -C embed_examples clean

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

$(OBJDIR)/vm.o:	$(SRCDIR)/opcodes.hh
$(SRCDIR)/opcodes.hh:	compiler/opcodes.txt
	echo "// Do not edit!\n\
// This file is generated from $<\n\
" > $@
	ruby -ne 'line = $$_.chomp; if !line.empty? && line[0] != ";" then op = line.split(" ")[0]; puts "OP(#{op})"; end' < $< >> $@

test:	$(PROJECT)
	make -C compiler test
	make -C test test
	cd examples && ./test.sh
	make -C embed_examples test

check-length:
	wc -l src/* include/**/*.hh compiler/*.yl | sort -nr

boot.bin:	self.bin
	mv self.bin boot.bin

self.bin:	compiler/boot.yl compiler/backquote.yl compiler/util.yl compiler/setf.yl compiler/node.yl compiler/compiler.yl
	./yalp -L boot.bin -C $^ | \
	   ./yalp -L boot.bin tools/optimize.yl > self.bin

self-compile:	self.bin
	diff boot.bin self.bin && rm self.bin && echo OK

include/yalp/binder.inc:	tools/gen_template.rb
	ruby $< > $@

dump_obj_size:	tools/src/dump_obj_size.cc $(OBJS)
	g++ $(CXXFLAGS) -o $@ $< $(LIBNAME)
