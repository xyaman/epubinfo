# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -fPIC

RELEASE_FLAGS = -O2
PKG = -I$(CURDIR)/include -lz

# Source files
SRC = $(wildcard src/*.c)
SRC_BIN = $(SRC) bin/main.c # binary
SRC_LIB = $(SRC)			# library

# Output directories
OUT_DIR = $(CURDIR)/out
BIN_OUT = $(OUT_DIR)/epubinfo
STATIC_LIB = $(OUT_DIR)/libepubinfo.a
SHARED_LIB = $(OUT_DIR)/libepubinfo.so

# Detect OS for dynamic library extension
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    SHARED_LIB = $(OUT_DIR)/libepubinfo.dylib
    SHARED_FLAGS = -dynamiclib
else

    SHARED_FLAGS = -shared
endif

all: lib-static lib-shared

lib-static: $(SRC_LIB:.c=.o)
	@mkdir -p $(OUT_DIR)
	ar rcs $(STATIC_LIB) $(SRC_LIB:.c=.o)

lib-shared: $(SRC_LIB:.c=.so.o)
	@mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) $(SHARED_FLAGS) $^ $(PKG) -o $(SHARED_LIB)

# Object files for static library
%.o: %.c
	$(CC) $(CFLAGS) $(PKG) -c $< -o $@

# Object files for shared library (position-independent)
%.so.o: %.c
	$(CC) $(CFLAGS) -fPIC $(PKG) -c $< -o $@

bin: $(SRC_BIN:.c=.o)
	@mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) $^ $(PKG) -o $(BIN_OUT)

# Release versions
lib-static-release: CFLAGS += $(RELEASE_FLAGS)
lib-static-release: lib-static

lib-shared-release: CFLAGS += $(RELEASE_FLAGS)
lib-shared-release: lib-shared

bin-release: CFLAGS += $(RELEASE_FLAGS)
bin-release: bin

clean:
	rm -rf $(OUT_DIR) src/*.o src/*.so.o bin/*.o
