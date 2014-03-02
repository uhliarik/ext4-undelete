app=ext-undelete

OBJ=main.o
CFLAGS=-O2 -D_FILE_OFFSET_BITS=64
CC=gcc

.PHONY: build
.PHONE: clean

build: ${app}

clean:
	rm -f *.o ${app}

${app}: ${OBJ}
	${CC} ${CFLAGS} ${OBJ} -o ${app} ${CGLAGS}

main.o: main.c main.h ext4.h 
	${CC} ${CFLAGS} -c main.c
