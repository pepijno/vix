#include "qbe.h"

struct qbe_type const qbe_byte = {
    .stype = QBE_STYPE_BYTE,
    .size  = 1,
};
struct qbe_type const qbe_half = {
    .stype = QBE_STYPE_HALF,
    .size  = 2,
};
struct qbe_type const qbe_word = {
    .stype = QBE_STYPE_WORD,
    .size  = 4,
};
struct qbe_type const qbe_long = {
    .stype = QBE_STYPE_LONG,
    .size  = 8,
};
struct qbe_type const qbe_single = {
    .stype = QBE_STYPE_SINGLE,
    .size  = 4,
};
struct qbe_type const qbe_double = {
    .stype = QBE_STYPE_DOUBLE,
    .size  = 8,
};
struct qbe_type const qbe_void = {
    .stype = QBE_STYPE_VOID,
};
struct qbe_type const qbe_aggregate = {
    .stype = QBE_STYPE_AGGREGATE,
};

void
qbe_append_definition(
    struct qbe_program program[static 1],
    struct qbe_definition definition[static 1]
) {
    *program->next = definition;
    program->next  = &definition->next;
}

struct qbe_value
const_u64(u64 value) {
    return (struct qbe_value){
        .value_type = QBE_VALUE_TYPE_CONSTANT,
        .type       = &qbe_long,
        .u64        = value,
    };
}
