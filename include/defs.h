#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef float f32;
typedef double f64;
typedef intptr_t iptr;
typedef uintptr_t uptr;
typedef size_t usize;

#define ALIGN_UNDEFINED ((usize) - 1)
