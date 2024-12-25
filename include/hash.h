#pragma once

#include "defs.h"

#define HASH_INIT 2166136261u

u32 fnv1a(u32 const hash, u8 const c);
u32 fnv1a_u32(u32 const hash, u32 const u);
u32 fnv1a_u64(u32 const hash, u64 const u);
