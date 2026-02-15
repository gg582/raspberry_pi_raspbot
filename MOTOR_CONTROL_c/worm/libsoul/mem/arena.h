#ifndef LIBTTAK_MEM_ARENA_H
#define LIBTTAK_MEM_ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t* base;
    size_t capacity;
    size_t offset;
} ttak_arena_t;

void ttak_arena_init(ttak_arena_t* arena, void* backing, size_t capacity);
void* ttak_arena_alloc(ttak_arena_t* arena, size_t size, size_t alignment);
void ttak_arena_reset(ttak_arena_t* arena);

#endif // LIBTTAK_MEM_ARENA_H
