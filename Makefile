PROJECT=macp

OBJS=\
	main.o\
	macp.o\
	symbol_manager.o\

CXXFLAGS += -Wall -Wextra -std=c++0x

all:	$(PROJECT)

clean:
	rm -f $(OBJS)
	rm -f $(PROJECT)

$(PROJECT):	$(OBJS)
	g++ -o $(PROJECT) $(OBJS)
