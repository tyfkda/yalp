PROJECT=yalp

OBJS=\
	main.o\
	object.o\
	read.o\
	symbol_manager.o\
	vm.o\
	yalp.o\

CXXFLAGS += -Wall -Wextra -Werror -std=c++0x

all:	$(PROJECT)

clean:
	rm -f $(OBJS)
	rm -f $(PROJECT)

$(PROJECT):	$(OBJS)
	g++ -o $(PROJECT) $(OBJS)
