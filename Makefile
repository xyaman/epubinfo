CC=gcc
CFLAGS=-Wall -Wextra -pedantic
RELEASE_FLAGS=-O2
PKG=-I./include -lz

SRC=main.c
OUT=./bin/epubinfo

all:
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) $(PKG) -o $(OUT)

release:
	mkdir -p bin
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) $(SRC) $(PKG) -o $(OUT)

clean:
	rm -rf bin
