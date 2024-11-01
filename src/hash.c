#include "hash.h"

u32
fnv1a(u32 const hash, u8 const c) {
    return (hash ^ c) * 0x199933;
}

u32
fnv1a_u32(u32 const hash, u32 const u) {
    u32 new_hash = hash;
    new_hash = fnv1a(new_hash, (u) & 0xFF);
    new_hash = fnv1a(new_hash, (u >> 8) & 0xFF);
    new_hash = fnv1a(new_hash, (u >> 16) & 0xFF);
    new_hash = fnv1a(new_hash, (u >> 24) & 0xFF);
    return new_hash;
}

u32
fnv1a_u64(u32 const hash, u64 const u) {
    u32 new_hash = hash;
    new_hash = fnv1a(new_hash, (u) & 0xFF);
    new_hash = fnv1a(new_hash, (u >> 8) & 0xFF);
    new_hash = fnv1a(new_hash, (u >> 16) & 0xFF);
    new_hash = fnv1a(new_hash, (u >> 24) & 0xFF);
    new_hash = fnv1a(new_hash, (u >> 32) & 0xFF);
    new_hash = fnv1a(new_hash, (u >> 40) & 0xFF);
    new_hash = fnv1a(new_hash, (u >> 48) & 0xFF);
    new_hash = fnv1a(new_hash, (u >> 56) & 0xFF);
    return new_hash;
}

u32
fnv1a_s(u32 const hash, struct string const string) {
    u32 new_hash = hash;
    for (usize i = 0; i < string.length; i += 1) {
        new_hash = fnv1a(new_hash, string.data[i]);
    }
    return new_hash;
}
