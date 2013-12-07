PROJECT=yalp

SRCPATH=src
INCPATH=include
OBJPATH=obj

SRCS=$(wildcard $(SRCPATH)/*.cc)
OBJS=$(subst $(SRCPATH),$(OBJPATH),$(SRCS:%.cc=%.o))
DEPS=$(subst $(SRCPATH),$(OBJPATH),$(SRCS:%.cc=%.d))

CXXFLAGS += -Wall -Wextra -Werror -std=c++0x -MMD -I$(INCPATH)

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


check-length:
	wc -l src/* include/**/* | sort -nr

boot.bin:	compiler/compiler.scm compiler/boot.arc compiler/self.arc
	cd compiler && gosh compiler.scm -c boot.arc self.arc > ../boot.bin

self-compile:
	./yalp -L boot.bin -c compiler/boot.arc compiler/self.arc > self.bin && \
	./yalp -L self.bin -c compiler/boot.arc compiler/self.arc > self2.bin && \
	diff self.bin self2.bin && rm self.bin self2.bin && echo OK
