#include "arena.h"

#include <string.h>

void ttak_arena_init(ttak_arena_t* arena, void* backing, size_t capacity) {
    arena->base = (uint8_t*)backing;
    arena->capacity = capacity;
    arena->offset = 0;
}

static size_t align_offset(size_t offset, size_t alignment) {
    size_t mask = alignment - 1;
    return (offset + mask) & ~mask;
}

void* ttak_arena_alloc(ttak_arena_t* arena, size_t size, size_t alignment) {
    size_t aligned_offset = align_offset(arena->offset, alignment);
    if (aligned_offset + size > arena->capacity) {
        return NULL;
    }
    void* ptr = arena->base + aligned_offset;
    memset(ptr, 0, size);
    arena->offset = aligned_offset + size;
    return ptr;
}

void ttak_arena_reset(ttak_arena_t* arena) {
    arena->offset = 0;
}
