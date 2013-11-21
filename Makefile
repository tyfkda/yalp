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
