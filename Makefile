CC = gcc
DEBUG=-g
OPT=-O0
PROGS = myshell
OBJS = observe.o reconstruct.o tapplot.o
SLIBS= sharedlibrary.so 
LDLIBS = -ldl
LD = -L.
%.o:%.c
	$(CC) -fPIC -c ${OPT} ${DEBUG} $< -o $@
.PHONY: all TAPPER TAPPET clean_tapper clean_tappet 
.SILENT: TAPPER TAPPET clean_tapper clean_tappet
all: myshell TAPPER TAPPET
TAPPER: tapper observe reconstruct tapplot
TAPPET: tappet sharedlibrary.so
myshell: $(OBJS)
	$(CC) ${OPT} ${DEBUG} $^ -o $@
tappet: tappet.o $(SLIBS)
	$(CC) $(LD) ${OPT} ${DEBUG} $^ $(LDLIBS) -lrt -lpthread -o $@  
tapper: tapper.o
	$(CC) ${OPT} ${DEBUG} $^ -lrt -lpthread -o $@  
observe: observe.o
	$(CC) ${OPT} ${DEBUG} $^ -lrt -lpthread -o $@ 
reconstruct: reconstruct.o
	$(CC) ${OPT} ${DEBUG} $^ -lrt -lpthread -o $@ 
tapplot: tapplot.o
	$(CC) ${OPT} ${DEBUG} $^ -lrt -lpthread -o $@ 
sharedlibrary.so: $(OBJS)
	$(CC) -shared -o $@ $^
clean:
	rm -f *.o *.so TAPPER TAPPET myshell observe tapper reconstruct tapplot
clean_tapper:
	rm -f *.o observe tapper reconstruct tapplot myshell
clean_tappet:
	rm -f *.o observe tappet reconstruct tapplot myshell