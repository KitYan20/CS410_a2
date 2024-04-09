CC = gcc
DEBUG=-g
OPT=-O0
PROGS = myshell
OBJS = myshell.o
%.o:%.c
	$(CC) -c ${OPT} ${DEBUG} $< -o $@
.PHONY: all clean
.SILENT: clean
all: myshell
TAPPER: tapper observe reconstruct
myshell: $(OBJS)
	$(CC) ${OPT} ${DEBUG} $^ -o $@
tapper: tapper.o
	$(CC) ${OPT} ${DEBUG} $^ -lrt -lpthread -o $@  
observe: observe.o
	$(CC) ${OPT} ${DEBUG} $^ -lrt -lpthread -o $@ 
reconstruct: reconstruct.o
	$(CC) ${OPT} ${DEBUG} $^ -lrt -lpthread -o $@ 
clean_tapper:
	rm *.o observe tapper reconstruct