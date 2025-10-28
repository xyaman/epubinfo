#include <stddef.h>

#ifndef ARENA_H
#define ARENA_H

typedef struct {
    unsigned char *buffer;
    size_t capacity;
    size_t offset;
} Arena;

int arena_init(Arena *arena, size_t size);
void* arena_alloc(Arena *arena, size_t size, size_t align);
void arena_reset(Arena *arena);
void arena_free(Arena *arena);

#endif
