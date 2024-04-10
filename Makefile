CC=clang -O3

CFLAGS=-Wall -g -Ilib/ffmpeg/include
LDFLAGS=-Llib/ffmpeg/lib -lavformat -lavcodec -lavutil -lswresample -lswscale

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
BIN=bin

.PHONY: all clean

all: streamline

run: all
	$(BIN)/streamline.exe

streamline: $(OBJ)
	$(CC) -o $(BIN)/streamline.exe $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	del /Q bin\streamline.exe src\*.o
