#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

#define LIST_OF(TYPE) struct { \
    TYPE *data; \
    size_t size; \
    size_t capacity; \
}

#define LIST_INIT_WITH(VEC, SIZE) (VEC).size = 0; \
(VEC).capacity = SIZE; \
(VEC).data = malloc(SIZE * sizeof(*(VEC).data));

#define LIST_INIT(VEC) LIST_INIT_WITH(VEC, 8)

#define LIST_SIZE(VEC) (VEC).size

#define LIST_CAPACITY(VEC) (VEC).capacity

#define LIST_PTR(VEC) (VEC).data

#define LIST_IS_EMPTY(VEC) (VEC).size == 0

#define LIST_IS_NON_EMPTY(VEC) (VEC).size != 0

#define LIST_RESIZE(VEC, SIZE) { \
  (VEC).data = realloc((VEC).data, SIZE * sizeof(*(VEC).data)); \
} 

#define LIST_PUSH_BACK(VEC, X) if (LIST_SIZE(VEC) == LIST_CAPACITY(VEC)) { \
  (VEC).capacity *= 2; \
  LIST_RESIZE((VEC), (VEC).capacity); \
} \
(VEC).data[LIST_SIZE(VEC)] = (X); \
(VEC).size += 1;

#define LIST_AT(VEC, N) (VEC).data[N]

#define LIST_FREE(VEC) free((VEC).data)

#endif
