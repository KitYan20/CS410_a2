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
myshell: $(OBJS)
	$(CC) ${OPT} ${DEBUG} $^ -o $@
tapper: tapper.o
	$(CC) ${OPT} ${DEBUG} -lrt $^ -o $@
observe: observe.o
	$(CC) ${OPT} ${DEBUG} -lrt $^ -o $@	
reconstruct: reconstruct.o
	$(CC) ${OPT} ${DEBUG} $^ -o $@
clean:
	rm *.o myshell observe tapper reconstruct