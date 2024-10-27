#pragma once

#include "defs.h"
#include "str.h"

enum qbe_stype {
    QBE_STYPE_VOID      = 'V',
    QBE_STYPE_BYTE      = 'b',
    QBE_STYPE_HALF      = 'h',
    QBE_STYPE_WORD      = 'w',
    QBE_STYPE_LONG      = 'l',
    QBE_STYPE_SINGLE    = 's',
    QBE_STYPE_DOUBLE    = 'd',
    QBE_STYPE_AGGREGATE = 'A',
    QBE_STYPE_UNION     = 'U',
};

struct qbe_type;

struct qbe_field {
    struct qbe_type* type;
    usize count;
    struct qbe_field* next;
};

struct qbe_type {
    enum qbe_stype stype;
    usize size;

    struct string name;
    struct qbe_field fields;
    struct type* base;
};

enum qbe_value_type {
    QBE_VALUE_TYPE_CONSTANT,
    QBE_VALUE_TYPE_GLOBAL,
    QBE_VALUE_TYPE_LABEL,
    QBE_VALUE_TYPE_TEMPORARY,
    QBE_VALUE_TYPE_VARIADIC,
};

struct qbe_value {
    enum qbe_value_type value_type;
    struct qbe_type const* type;
    union {
        struct string name;
        u32 u32;
        u64 u64;
        f32 f32;
        f64 f64;
    };
};

enum qbe_instruction {
    Q_ADD,
    Q_ALLOC16,
    Q_ALLOC4,
    Q_ALLOC8,
    Q_AND,
    Q_BLIT,
    Q_CALL,
    Q_CAST,
    Q_CEQD,
    Q_CEQL,
    Q_CEQS,
    Q_CEQW,
    Q_CGED,
    Q_CGES,
    Q_CGTD,
    Q_CGTS,
    Q_CLED,
    Q_CLES,
    Q_CLTD,
    Q_CLTS,
    Q_CNED,
    Q_CNEL,
    Q_CNES,
    Q_CNEW,
    Q_COD,
    Q_COPY,
    Q_COS,
    Q_CSGEL,
    Q_CSGEW,
    Q_CSGTL,
    Q_CSGTW,
    Q_CSLEL,
    Q_CSLEW,
    Q_CSLTL,
    Q_CSLTW,
    Q_CUGEL,
    Q_CUGEW,
    Q_CUGTL,
    Q_CUGTW,
    Q_CULEL,
    Q_CULEW,
    Q_CULTL,
    Q_CULTW,
    Q_CUOD,
    Q_CUOS,
    Q_DBGLOC,
    Q_DIV,
    Q_DTOSI,
    Q_DTOUI,
    Q_EXTS,
    Q_EXTSB,
    Q_EXTSH,
    Q_EXTSW,
    Q_EXTUB,
    Q_EXTUH,
    Q_EXTUW,
    Q_HLT,
    Q_JMP,
    Q_JNZ,
    Q_LOADD,
    Q_LOADL,
    Q_LOADS,
    Q_LOADSB,
    Q_LOADSH,
    Q_LOADSW,
    Q_LOADUB,
    Q_LOADUH,
    Q_LOADUW,
    Q_MUL,
    Q_OR,
    Q_REM,
    Q_RET,
    Q_NEG,
    Q_SAR,
    Q_SHL,
    Q_SHR,
    Q_SLTOF,
    Q_STOREB,
    Q_STORED,
    Q_STOREH,
    Q_STOREL,
    Q_STORES,
    Q_STOREW,
    Q_STOSI,
    Q_STOUI,
    Q_SUB,
    Q_SWTOF,
    Q_TRUNCD,
    Q_UDIV,
    Q_ULTOF,
    Q_UREM,
    Q_UWTOF,
    Q_VAARG,
    Q_VASTART,
    Q_XOR,

    Q_LAST_INSTR,
};

struct qbe_argument {
    struct qbe_value value;
    struct qbe_argument* next;
};

enum qbe_statement_type {
    QBE_STATEMENT_TYPE_COMMENT,
    QBE_STATEMENT_TYPE_INSTRUCTION,
    QBE_STATEMENT_TYPE_LABEL,
};

struct qbe_statement {
    enum qbe_statement_type statement_type;
    union {
        struct {
            enum qbe_instruction instruction;
            struct qbe_value* out;
            struct qbe_argument* arguments;
        };
        struct string label;
        struct string comment;
    };
};

struct qbe_statements {
    usize length;
    usize size;
    struct qbe_statement* statements;
};

struct qbe_function {
    struct qbe_type* returns;
    struct qbe_function_parameters* parameters;
    bool variadic;
    struct qbe_statements prelude;
    struct qbe_statements body;
};

enum qbe_data_type {
    QBE_DATA_TYPE_ZEROED,
    QBE_DATA_TYPE_VALUE,
    QBE_DATA_TYPE_STRINGS,
    QBE_DATA_TYPE_SYMBOLS_OFFSETS,
};

struct qbe_data_item {
    enum qbe_data_type data_type;
    union {
        struct qbe_value value;
        usize zeroed;
        struct string string;
        struct {
            u8* symbols;
            i64 offsets;
        };
    };
    struct qbe_data_item* next;
};

struct qbe_data {
    usize align;
    u8* section;
    u8* section_flags;
    struct qbe_data_item items;
};

enum qbe_definition_type {
    QBE_DEFINITION_TYPE_FUNCTION,
    QBE_DEFINITION_TYPE_TYPE,
    QBE_DEFINITION_TYPE_DATA,
};

struct qbe_definition {
    struct string name;
    enum qbe_definition_type definition_type;
    union {
        struct qbe_function function;
        struct qbe_type type;
        struct qbe_data data;
    };
    struct qbe_definition* next;
};

struct qbe_program {
    struct qbe_definition* definitions;
    struct qbe_definition** next;
};

void qbe_append_definition(
    struct qbe_program program[static 1],
    struct qbe_definition definition[static 1]
);

extern struct qbe_type const qbe_byte;
extern struct qbe_type const qbe_half;
extern struct qbe_type const qbe_word;
extern struct qbe_type const qbe_long;
extern struct qbe_type const qbe_single;
extern struct qbe_type const qbe_double;
extern struct qbe_type const qbe_void;
extern struct qbe_type const qbe_aggregate;

struct qbe_value const_u64(u64 value);
