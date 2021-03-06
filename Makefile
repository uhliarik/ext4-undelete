app=ext4-undelete

OBJ=main.o ext4_undelete.o
#DEBUG=-D DEBUG
CDEFINE=-D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
CFLAGS=-std=c99 -Wextra -Wall -Werror -pedantic -O2 ${CDEFINE} ${DEBUG}
CC=gcc
LIB=-lext2fs

.PHONY: build
.PHONE: clean

build: ${app}

clean:
	rm -f *.o ${app}

${app}: ${OBJ}
	${CC} ${CFLAGS} ${OBJ} -o ${app} ${CGLAGS} ${LIB}

main.o: main.c main.h
	${CC} ${CFLAGS} -c main.c

ext4_undelete.o: ext4_undelete.c ext4_undelete.h
	${CC} ${CFLAGS} -c ext4_undelete.c