CC=g++

CFLAGS= -g -O3 -Iinclude
LDFLAGS=

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
BIN=bin

.PHONY: all shared clean

all: streamline

run: all
	$(BIN)/streamline.exe

streamline: $(OBJ)
	$(CC) -o $(BIN)/streamline.exe $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

static: $(OBJ)
	ar rcs $(BIN)/libstreamline.a $^

clean:
	del /Q bin\streamline.exe src\*.o
