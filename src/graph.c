#include "graph.h"

#include "allocator.h"
#include "hash.h"
#include "hashmap.h"
#include "hashset.h"
#include "queue.h"

static u32
usize_hash(usize const u) {
    return fnv1a_u64(HASH_INIT, u);
}
static u32
usize_equals(usize const u1, usize const u2) {
    return u1 == u2;
}
HASHSET_IMPL(usize, sizes, usize_hash, usize_equals)
VECTOR_IMPL(struct group*, group_ptr)

static u32
edge_hash(struct edge edge) {
    return fnv1a_u64(fnv1a_u64(HASH_INIT, edge.from), edge.to);
}

static bool
edge_equals(struct edge edge1, struct edge edge2) {
    return edge1.from == edge2.from && edge1.to == edge2.to;
}

HASHSET_IMPL(struct edge, edge, edge_hash, edge_equals)
HASHMAP_IMPL(usize, struct hashset_sizes*, adjacency, usize_hash, usize_equals)

QUEUE_DEF(usize, sizes)
QUEUE_IMPL(usize, sizes)

static struct hashset_edge
graph_compute_transitive_edges(struct object_graph obj_graph) {
    struct hashset_edge transitive_edges = hashset_edge_copy(obj_graph.edges);
    hashmap_foreach(obj_graph.adjacency_lists, connector) {
        hashmap_foreach(obj_graph.adjacency_lists, from) {
            struct edge to_connector = {
                .from = from.key,
                .to   = connector.key,
            };
            hashmap_foreach(obj_graph.adjacency_lists, to) {
                struct edge full_jump = {
                    .from = from.key,
                    .to   = to.key,
                };

                if (hashset_edge_contains(transitive_edges, full_jump)) {
                    continue;
                }

                struct edge from_connector = {
                    .from = connector.key,
                    .to   = to.key,
                };

                if (hashset_edge_contains(transitive_edges, to_connector)
                    && hashset_edge_contains(
                        transitive_edges, from_connector
                    )) {
                    hashset_edge_add(&transitive_edges, full_jump);
                }
            }
        }
    }

    return transitive_edges;
}

HASHMAP_DEFS(usize, usize, group_ids)
HASHMAP_IMPL(usize, usize, group_ids, usize_hash, usize_equals)
HASHMAP_DEFS(usize, struct group_data*, group_data)
HASHMAP_IMPL(usize, struct group_data*, group_data, usize_hash, usize_equals)

static void
graph_create_groups(
    struct arena* arena, struct object_graph obj_graph,
    struct hashset_edge transitive_edges, struct hashmap_group_ids* group_ids,
    struct hashmap_group_data* group_data_map
) {
    usize id_counter = 0;

    hashmap_foreach(obj_graph.adjacency_lists, vertex) {
        if (hashmap_group_ids_contains(*group_ids, vertex.key)) {
            continue;
        }

        struct group_data* new_group
            = arena_allocate(arena, sizeof(struct group_data));
        new_group->functions      = hashset_sizes_new(arena);
        new_group->adjacency_list = hashset_sizes_new(arena);
        hashset_sizes_add(&new_group->functions, vertex.key);
        hashmap_group_data_add(group_data_map, id_counter, new_group);
        hashmap_group_ids_add(group_ids, vertex.key, id_counter);
        hashmap_foreach(obj_graph.adjacency_lists, other_vertex) {
            if (hashset_edge_contains(
                    transitive_edges,
                    (struct edge){ .from = vertex.key, .to = other_vertex.key }
                )
                && hashset_edge_contains(
                    transitive_edges, (struct edge){ .from = other_vertex.key,
                                                     .to   = other_vertex.key }
                )) {
                hashmap_group_ids_add(group_ids, other_vertex.key, id_counter);
                hashset_sizes_add(&new_group->functions, other_vertex.key);
            }
        }
        id_counter += 1;
    }
}

struct pair {
    u64 from;
    u64 to;
};

HASHSET_DEFS(struct pair, pair)
static u32
pair_hash(struct pair pair) {
    return fnv1a_u64(fnv1a_u64(HASH_INIT, pair.from), pair.to);
}
static bool
pair_equals(struct pair pair1, struct pair pair2) {
    return pair1.from == pair2.from && pair1.to == pair2.to;
}
HASHSET_IMPL(struct pair, pair, pair_hash, pair_equals)

static void
graph_create_edges(
    struct arena* arena, struct object_graph* obj_graph,
    struct hashmap_group_ids* group_ids,
    struct hashmap_group_data* group_data_map
) {
    struct hashset_pair group_edges = hashset_pair_new(arena);

    hashmap_foreach(obj_graph->adjacency_lists, vertex) {
        auto vertex_id   = hashmap_group_ids_find(group_ids, vertex.key);
        auto vertex_data = hashmap_group_data_find(group_data_map, vertex_id);
        hashset_foreach(*vertex.value, other_vertex) {
            auto other_id = hashmap_group_ids_find(group_ids, other_vertex);
            if (vertex_id == other_id) {
                continue;
            }
            if (hashset_pair_contains(
                    group_edges,
                    (struct pair){ .from = vertex_id, .to = other_id }
                )) {
                continue;
            }
            hashset_pair_add(
                &group_edges, (struct pair){ .from = vertex_id, .to = other_id }
            );
            hashset_sizes_add(&vertex_data->adjacency_list, other_id);
            hashmap_group_data_find(group_data_map, other_id)->indegree += 1;
        }
    }
}

static struct vector_group_ptr
graph_generate_order(
    struct arena* arena, struct hashmap_group_data* group_data_map
) {
    struct queue_sizes id_queue    = queue_sizes_new(arena);
    struct vector_group_ptr output = vector_group_ptr_new(arena);

    hashmap_foreach(*group_data_map, group) {
        if (group.value->indegree == 0) {
            queue_sizes_push(&id_queue, group.key);
        }
    }

    while (!queue_sizes_is_empty(id_queue)) {
        auto new_id     = queue_sizes_pop(&id_queue);
        auto group_data = hashmap_group_data_find(group_data_map, new_id);
        struct group* output_group
            = arena_allocate(arena, sizeof(struct group));
        output_group->members = hashset_sizes_copy(group_data->functions);

        hashset_foreach(group_data->adjacency_list, adjacenct_group) {
            auto data
                = hashmap_group_data_find(group_data_map, adjacenct_group);
            data->indegree -= 1;
            if (data->indegree == 0) {
                queue_sizes_push(&id_queue, adjacenct_group);
            }
        }

        vector_group_ptr_add(&output, output_group);
    }

    return output;
}

struct vector_group_ptr
graph_compute_order(struct arena* arena, struct object_graph* obj_graph) {
    struct hashset_edge transitive_edges
        = graph_compute_transitive_edges(*obj_graph);
    struct hashmap_group_ids group_ids       = hashmap_group_ids_new(arena);
    struct hashmap_group_data group_data_map = hashmap_group_data_new(arena);
    graph_create_groups(
        arena, *obj_graph, transitive_edges, &group_ids, &group_data_map
    );
    graph_create_edges(arena, obj_graph, &group_ids, &group_data_map);
    return graph_generate_order(arena, &group_data_map);
}

struct hashset_sizes*
graph_add_function(
    struct arena* arena, struct object_graph* obj_graph, usize const function
) {
    if (hashmap_adjacency_contains(obj_graph->adjacency_lists, function)) {
        return hashmap_adjacency_find(&obj_graph->adjacency_lists, function);
    } else {
        auto new_set                   = hashset_string_new(arena);
        struct hashset_sizes* heap_set = arena_allocate(arena, sizeof(new_set));
        memcpy(heap_set, &new_set, sizeof(new_set));
        hashmap_adjacency_add(&obj_graph->adjacency_lists, function, heap_set);
        return heap_set;
    }
}

void
graph_add_edge(
    struct arena* arena, struct object_graph* obj_graph, usize const from,
    usize const to
) {
    struct hashset_sizes* set = graph_add_function(arena, obj_graph, from);
    hashset_sizes_add(set, to);
    hashset_edge_add(
        &obj_graph->edges, (struct edge){ .from = from, .to = to }
    );
}
