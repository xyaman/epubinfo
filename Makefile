CC=gcc
CFLAGS=-Wall -Wextra -pedantic
RELEASE_FLAGS=-O2
PKG=-lz

SRC=main.c
OUT=.bin/epubinfo

all:
	mkdir -p bin
	$(CC) $(CFLAGS) $(PKG) $(SRC) -o $(OUT)

release:
	mkdir -p bin
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) $(PKG) $(SRC) -o $(OUT)

clean:
	rm -rf bin
