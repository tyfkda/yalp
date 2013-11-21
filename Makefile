PROJECT=yalp

SRCS=$(wildcard *.cc)
OBJS=$(SRCS:%.cc=%.o)
DEPS:=$(SRCS:%.cc=%.d)

CXXFLAGS += -Wall -Wextra -Werror -std=c++0x -MMD

all:	$(PROJECT)

clean:
	rm -f $(OBJS) $(DEPS)
	rm -f $(PROJECT)

-include $(DEPS)

$(PROJECT):	$(OBJS)
	g++ -o $(PROJECT) $(OBJS)
