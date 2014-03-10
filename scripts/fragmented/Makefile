app=fragmented

OBJ=fragmented.o
CDEFINE=
CFLAGS=-std=c99 -Wextra -Wall -Werror -pedantic -O2 ${CDEFINE}
CC=gcc

.PHONY: build
.PHONE: clean

build: ${app}

clean:
	rm -f *.o ${app}

${app}: ${OBJ}
	${CC} ${CFLAGS} ${OBJ} -o ${app} ${CGLAGS}

fragmented.o: fragmented.c fragmented.h
	${CC} ${CFLAGS} -c fragmented.c