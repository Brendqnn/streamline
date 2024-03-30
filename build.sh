#!/bin/bash

CFLAGS="-Wall -g"
LDFLAGS="-lavformat -lavcodec -lavutil -lasound -lswresample -lm"

SOURCE_FILES="src/*.c"
OUTPUT_EXE="streamline"

clang $CFLAGS -o $OUTPUT_EXE $SOURCE_FILES $LDFLAGS
