CC = gcc
CFLAGS = -std=c99 -I.

DEPS = dict.h gcode.h main.h chr.h
FILES = dict.c main.c chr.c
LIBS = -lm

OUT = chr-gcode

all: $(OUT)

$(OUT): $(DEPS) $(FILES)
	$(CC) $(CFLAGS) $(LIBS) -o $(OUT) $(FILES)

.PHONY: clean

clean:
	rm -rf $(OUT) *.o
