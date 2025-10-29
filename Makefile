CC=gcc
CFLAGS=-Wall -Wextra -pedantic -fPIC
RELEASE_FLAGS=-O2
PKG=-I./include -lz

SRC_BIN=$(wildcard src/*.c)
SRC_LIB=$(filter-out src/main.c, $(SRC_BIN))

OUT_DIR=./out
BIN_OUT=$(OUT_DIR)/epubinfo
LIB_OUT=$(OUT_DIR)/libepubinfo.so

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LIB_OUT=$(OUT_DIR)/libepubinfo.dylib
    SHARED_FLAG=-dynamiclib
else
    SHARED_FLAG=-shared
endif

all: bin lib

bin: $(SRC_BIN)
	mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) $(SRC_BIN) $(PKG) -o $(BIN_OUT)

lib: $(SRC_LIB)
	mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) $(SHARED_FLAG) $(SRC_LIB) $(PKG) -o $(LIB_OUT)

bin-release: CFLAGS += $(RELEASE_FLAGS)
bin-release: bin

lib-release: CFLAGS += $(RELEASE_FLAGS)
lib-release: lib

clean:
	rm -rf $(OUT_DIR)
