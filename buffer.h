#ifndef BUFFER_H
#define BUFFER_H

#include "list.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>

typedef struct {
  LIST_OF(char) list;
} Buffer;

static inline size_t buffer_length(const Buffer *buffer) {
  assert(LIST_SIZE(buffer->list));
  return LIST_SIZE(buffer->list) - 1;
}

static inline char *buffer_ptr(const Buffer *buffer) {
  assert(LIST_SIZE(buffer->list));
  return LIST_PTR(buffer->list);
}

static inline void buffer_resize(Buffer *buffer, size_t new_length) {
  LIST_RESIZE(buffer->list, new_length + 1);
  LIST_AT(buffer->list, buffer_length(buffer)) = 0;
}

static inline Buffer *buffer_allocate_fixed(size_t size) {
  Buffer *buffer = (Buffer *)malloc(sizeof(Buffer));
  buffer_resize(buffer, size);
  return buffer;
}

static inline Buffer *buffer_allocate(void) { return buffer_allocate_fixed(0); }

static inline void buffer_deinit(Buffer *buffer) { LIST_FREE(buffer->list); }

static inline void buffer_init_from_memory(Buffer *buffer, const char *mem,
                                           size_t length) {
  LIST_RESIZE(buffer->list, length + 1);
  memcpy(buffer_ptr(buffer), mem, length);
  LIST_AT(buffer->list, buffer_length(buffer)) = 0;
}

static inline void buffer_init_from_string(Buffer *buffer, const char *str) {
  buffer_init_from_memory(buffer, str, strlen(str));
}

static inline void buffer_init_from_buffer(Buffer *buffer,
                                           const Buffer *other) {
  buffer_init_from_memory(buffer, buffer_ptr(other), buffer_length(other));
}

static inline Buffer *buffer_create_from_memory(const char *mem,
                                                size_t length) {
  Buffer *buffer = (Buffer *)malloc(sizeof(Buffer));
  buffer_init_from_memory(buffer, mem, length);
  return buffer;
}

static inline Buffer *buffer_create_from_string(const char *str) {
  return buffer_create_from_memory(str, strlen(str));
}

static inline Buffer *buffer_create_from_buffer(Buffer *buffer) {
  return buffer_create_from_memory(buffer_ptr(buffer), buffer_length(buffer));
}

static inline Buffer *buffer_slice(Buffer *buffer_in, size_t start,
                                   size_t end) {
  assert(LIST_SIZE(buffer_in->list));
  assert(start < buffer_length(buffer_in));
  assert(end >= buffer_length(buffer_in));
  Buffer *buffer_out = (Buffer *)malloc(sizeof(Buffer));
  buffer_resize(buffer_out, end - start + 1);
  memcpy(buffer_ptr(buffer_out), buffer_ptr(buffer_in) + start, end - start);
  LIST_AT(buffer_out->list, buffer_length(buffer_out)) = 0;
  return buffer_out;
}

static inline void buffer_append_memory(Buffer *buffer, const char *mem,
                                        size_t length) {
  assert(LIST_SIZE(buffer->list));
  size_t old_length = LIST_SIZE(buffer->list);
  buffer_resize(buffer, old_length + length);
  memcpy(buffer_ptr(buffer) + old_length, mem, length);
  LIST_AT(buffer->list, buffer_length(buffer)) = 0;
}

static inline void buffer_append_string(Buffer *buffer, const char *str) {
  buffer_append_memory(buffer, str, strlen(str));
}

static inline void buffer_append_buffer(Buffer *buffer, const Buffer *other) {
  buffer_append_memory(buffer, buffer_ptr(other), buffer_length(other));
}

static inline void buffer_append_char(Buffer *buffer, char c) {
  buffer_append_memory(buffer, (const char *)&c, 1);
}

static inline bool buffer_equal_memory(const Buffer *buffer, const char *mem,
                                       size_t length) {
  if (buffer_length(buffer) != length) {
    return false;
  }
  return memcmp(buffer_ptr(buffer), mem, length) == 0;
}

static inline bool buffer_equal_string(const Buffer *buffer, const char *str) {
  return buffer_equal_memory(buffer, str, strlen(str));
}

static inline bool buffer_equal_buffer(const Buffer *buffer,
                                       const Buffer *other) {
  return buffer_equal_memory(buffer, buffer_ptr(other), buffer_length(other));
}

static inline bool buffer_starts_with_memory(const Buffer *buffer,
                                             const char *mem, size_t length) {
  if (buffer_length(buffer) < length) {
    return false;
  }
  return memcmp(buffer, mem, length);
}

static inline bool buffer_starts_with_string(const Buffer *buffer,
                                             const char *str) {
  return buffer_starts_with_memory(buffer, str, strlen(str));
}

static inline bool buffer_starts_with_buffer(const Buffer *buffer,
                                             const Buffer *other) {
  return buffer_starts_with_memory(buffer, buffer_ptr(other),
                                   buffer_length(other));
}

static inline bool buffer_ends_with_memory(const Buffer *buffer,
                                           const char *mem, size_t length) {
  if (buffer_length(buffer) < length) {
    return false;
  }
  return memcmp(buffer_ptr(buffer) + buffer_length(buffer) - length, mem,
                length) == 0;
}

static inline bool buffer_ends_with_string(const Buffer *buffer,
                                           const char *str) {
  return buffer_ends_with_memory(buffer, str, strlen(str));
}

static inline bool buffer_ends_with_buffer(const Buffer *buffer,
                                           const Buffer *other) {
  return buffer_ends_with_memory(buffer, buffer_ptr(other),
                                 buffer_length(other));
}

static inline void buffer_uppercase(Buffer *buffer) {
  for (size_t i = 0; i < buffer_length(buffer); ++i) {
    buffer_ptr(buffer)[i] = (char)toupper(buffer_ptr(buffer)[i]);
  }
}

#endif
