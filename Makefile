app=ext-undelete

OBJ=main.o
OPT=-O2
CC=gcc

.PHONY: build
.PHONE: clean

build: ${app}

clean:
	rm -f *.o ${app}

${app}: ${OBJ}
	${CC} ${OBJ} -o ${app} ${OPT}

main.o: main.c
	${CC} main.c -c ${OPT}
