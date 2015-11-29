
# CFLAGS=-g -Wall -pg
CFLAGS=-O2
CXXFLAGS=-g -Wall
OBJS=main.o 
EXE=iosXMLParser

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) -o $@ $(OBJS)

clean:
	rm -f *.o $(EXE)

makedepend:
	makedepend *.c

