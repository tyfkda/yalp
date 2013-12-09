PROJECT=yalp

SRCPATH=src
INCPATH=include
OBJPATH=obj

SRCS=$(wildcard $(SRCPATH)/*.cc)
OBJS=$(subst $(SRCPATH),$(OBJPATH),$(SRCS:%.cc=%.o))
DEPS=$(subst $(SRCPATH),$(OBJPATH),$(SRCS:%.cc=%.d))

CXXFLAGS += -Wall -Wextra -Werror -std=c++0x -MMD -I$(INCPATH)

.PHONY: all clean test

all:	$(PROJECT)

clean:
	rm -rf $(OBJPATH)
	rm -f $(PROJECT)

-include $(DEPS)

$(PROJECT):	$(OBJS)
	g++ -o $(PROJECT) $(OBJS)

$(OBJPATH)/%.o:	$(SRCPATH)/%.cc
	@if ! [ -e $(OBJPATH) ]; then\
	  mkdir $(OBJPATH);\
	fi
	g++ $(CXXFLAGS) -o $@ -c $<

test:	$(PROJECT)
	make -C test

check-length:
	wc -l src/* include/**/* compiler/*.arc | sort -nr

boot.bin:	self.bin
	mv self.bin boot.bin

self.bin:	compiler/boot.arc compiler/self.arc
	./yalp -L boot.bin -c compiler/boot.arc compiler/self.arc > self.bin

self-compile:	self.bin
	diff boot.bin self.bin && rm self.bin && echo OK
