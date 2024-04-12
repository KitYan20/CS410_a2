CC = gcc
DEBUG=-g
OPT=-O0
PROGS = myshell
OBJS = myshell.o
%.o:%.c
	$(CC) -c ${OPT} ${DEBUG} $< -o $@
.PHONY: all TAPPER TAPPET clean_tapper clean_tappet 
.SILENT: TAPPER TAPPET clean_tapper clean_tappet
all: myshell
TAPPER: tapper observe reconstruct tapplot
TAPPET: tappet observe reconstruct tapplot
myshell: $(OBJS)
	$(CC) ${OPT} ${DEBUG} $^ -o $@
tappet: tappet.o
	$(CC) ${OPT} ${DEBUG} $^ -lrt -lpthread -o $@  
tapper: tapper.o
	$(CC) ${OPT} ${DEBUG} $^ -lrt -lpthread -o $@  
observe: observe.o
	$(CC) ${OPT} ${DEBUG} $^ -lrt -lpthread -o $@ 
reconstruct: reconstruct.o
	$(CC) ${OPT} ${DEBUG} $^ -lrt -lpthread -o $@ 
tapplot: tapplot.o
	$(CC) ${OPT} ${DEBUG} $^ -lrt -lpthread -o $@ 
clean_tapper:
	rm *.o observe tapper reconstruct tapplot
clean_tappet:
	rm *.o observe tappet reconstruct tapplot	