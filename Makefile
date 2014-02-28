app=ext-undelete

OBJ=main.o
CFLAGS=-O2
CC=gcc

.PHONY: build
.PHONE: clean

build: ${app}

clean:
	rm -f *.o ${app}

${app}: ${OBJ}
	${CC} ${CFLAGS} ${OBJ} -o ${app} ${CGLAGS}

main.o: main.c main.h
	${CC} ${CFLAGS} -c main.c
