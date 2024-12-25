#pragma once

#include "defs.h"
#include "hashmap.h"
#include "hashset.h"
#include "str.h"

HASHSET_DEFS(usize, sizes)

struct group {
    struct hashset_sizes members;
};

VECTOR_DEFS(struct group*, group_ptr)

struct group_data {
    struct hashset_sizes functions;
    struct hashset_sizes adjacency_list;
    usize indegree;
};

struct edge {
    usize from;
    usize to;
};

HASHSET_DEFS(struct edge, edge)
HASHMAP_DEFS(usize, struct hashset_sizes*, adjacency)

struct object_graph {
    struct hashmap_adjacency adjacency_lists;
    struct hashset_edge edges;
};

struct arena;

struct vector_group_ptr graph_compute_order(
    struct arena* arena, struct object_graph* obj_graph
);
struct hashset_sizes* graph_add_function(
    struct arena* arena, struct object_graph* obj_graph,
    usize const function
);
void graph_add_edge(
    struct arena* arena, struct object_graph* obj_graph,
    usize const from, usize const to
);
