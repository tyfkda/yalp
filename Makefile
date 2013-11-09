PROJECT=macp

OBJS=\
	main.o\
	macp.o\

all:	$(PROJECT)

clean:
	rm -f $(OBJS)
	rm -f $(PROJECT)

$(PROJECT):	$(OBJS)
	g++ -o $(PROJECT) $(OBJS)
