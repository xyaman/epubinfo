#include <stdint.h>
#include <stdlib.h>
#include "arena.h"

static inline uintptr_t align_forward(uintptr_t ptr, size_t alignment) {
    size_t mask = alignment - 1;
    return (ptr + mask) & ~mask;
}

int arena_init(Arena *arena, size_t size) {
    arena->buffer = malloc(size);
    if (!arena->buffer) return 0;
    arena->capacity = size;
    arena->offset = 0;
    return 1;
}

void arena_free(Arena *arena) {
    free(arena->buffer);
    arena->buffer = NULL;
    arena->offset = 0;
    arena->capacity = 0;
}

void* arena_alloc(Arena *arena, size_t size, size_t alignment) {
    uintptr_t current = (uintptr_t)(arena->buffer + arena->offset);
    uintptr_t aligned = align_forward(current, alignment);
    size_t new_offset = (aligned - (uintptr_t)arena->buffer) + size;

    if (new_offset > arena->capacity) {
        return NULL;
    }

    arena->offset = new_offset;
    return (void *)aligned;
}

void arena_reset(Arena *arena) {
    arena->offset = 0;
}
