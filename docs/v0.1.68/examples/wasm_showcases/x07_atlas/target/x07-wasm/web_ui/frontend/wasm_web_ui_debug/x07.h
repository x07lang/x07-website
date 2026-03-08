
#ifndef X07_PKG_H
#define X07_PKG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t* ptr;
  uint32_t len;
} bytes_t;

typedef struct {
  void* ctx;
  void* (*alloc)(void* ctx, uint32_t size, uint32_t align);
  void* (*realloc)(void* ctx, void* ptr, uint32_t old_size, uint32_t new_size, uint32_t align);
  void (*free)(void* ctx, void* ptr, uint32_t size, uint32_t align);
} allocator_v1_t;

allocator_v1_t x07_custom_allocator(void);

bytes_t x07_solve_v2(
    uint8_t* arena_mem,
    uint32_t arena_cap,
    const uint8_t* input_ptr,
    uint32_t input_len
);

int32_t x07_exit_code_v1(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // X07_PKG_H
