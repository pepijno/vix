#include "emit.h"

#include "qbe.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

static void
emit_qtype(struct qbe_type const* type, bool aggregate, FILE* out) {
    switch (type->stype) {
        case QBE_STYPE_VOID:
            break;
        case QBE_STYPE_BYTE:
        case QBE_STYPE_HALF:
        case QBE_STYPE_WORD:
        case QBE_STYPE_LONG:
        case QBE_STYPE_SINGLE:
        case QBE_STYPE_DOUBLE:
            fprintf(out, "%c", (char) type->stype);
            break;
        case QBE_STYPE_AGGREGATE:
        case QBE_STYPE_UNION:
            if (aggregate) {
                fprintf(out, ":%s", type->name.buffer);
            } else {
                fprintf(out, "l");
            }
            break;
    }
}

void
qemit_type(struct qbe_definition definition[static 1], FILE out[static 1]) {
    assert(definition->definition_type == QBE_DEFINITION_TYPE_TYPE);
    struct qbe_type* type = &definition->type;
    fprintf(out, "type :%s =", definition->name.buffer);
    fprintf(out, " {");

    struct qbe_field* field = &definition->type.fields;
    while (field) {
        if (type->stype == QBE_STYPE_UNION) {
            fprintf(out, " {");
        }
        if (field->type) {
            fprintf(out, " ");
            emit_qtype(field->type, true, out);
        }
        if (field->count) {
            fprintf(out, " %zu", field->count);
        }
        if (type->stype == QBE_STYPE_UNION) {
            fprintf(out, " }");
        } else if (field->next) {
            fprintf(out, ",");
        }
        field = field->next;
    }

    fprintf(out, " }\n\n");
}

static void
emit_constant(struct qbe_value value[static 1], FILE out[static 1]) {
    switch (value->type->stype) {
        case QBE_STYPE_BYTE:
        case QBE_STYPE_HALF:
        case QBE_STYPE_WORD:
        case QBE_STYPE_SINGLE:
            fprintf(out, "%u", value->u32);
            break;
        case QBE_STYPE_LONG:
        case QBE_STYPE_DOUBLE:
            fprintf(out, "%li", value->u64);
            break;
        case QBE_STYPE_VOID:
        case QBE_STYPE_AGGREGATE:
        case QBE_STYPE_UNION:
            assert(false);
    }
}

static void
emit_value(struct qbe_value value[static 1], FILE out[static 1]) {
    switch (value->value_type) {
        case QBE_VALUE_TYPE_CONSTANT:
            emit_constant(value, out);
            break;
        case QBE_VALUE_TYPE_GLOBAL:
            fprintf(out, "$%s", value->name.buffer);
            break;
        case QBE_VALUE_TYPE_LABEL:
            fprintf(out, "@%s", value->name.buffer);
            break;
        case QBE_VALUE_TYPE_TEMPORARY:
            fprintf(out, "%%%s", value->name.buffer);
            break;
        case QBE_VALUE_TYPE_VARIADIC:
            fprintf(out, "...");
            break;
    }
}

static bool
is_zeroes(struct qbe_data_item item[static 1]) {
    for (struct qbe_data_item* current = item; current != nullptr;
         current                       = current->next) {
        switch (current->data_type) {
            case QBE_DATA_TYPE_ZEROED:
                break;
            case QBE_DATA_TYPE_VALUE:
                switch (current->value.value_type) {
                    case QBE_VALUE_TYPE_CONSTANT:
                        if (current->value.type->size < sizeof(u64)) {
                            if (current->value.u32 != 0) {
                                return false;
                            }
                        } else {
                            if (current->value.u64 != 0) {
                                return false;
                            }
                        }
                        break;
                    case QBE_VALUE_TYPE_GLOBAL:
                    case QBE_VALUE_TYPE_LABEL:
                    case QBE_VALUE_TYPE_TEMPORARY:
                        return false;
                    case QBE_VALUE_TYPE_VARIADIC:
                        abort();
                }
                break;
            case QBE_DATA_TYPE_STRINGS:
                for (usize i = 0; i < current->string.length; i += 1) {
                    if (current->string.buffer[i] != 0) {
                        return false;
                    }
                }
                break;
            case QBE_DATA_TYPE_SYMBOLS_OFFSETS:
                return false;
        }
    }
    return true;
}

static void
emit_data_string(struct string string, FILE out[static 1]) {
    bool q = false;
    for (usize i = 0; i < string.length; i += 1) {
        if (!isprint((unsigned char) (string.buffer[i]))
            || string.buffer[i] == '"' || string.buffer[i] == '\\') {
            if (q) {
                q = false;
                fprintf(out, "\", ");
            }
            fprintf(
                out, "b %d%s", string.buffer[i],
                i + 1 < string.length ? ", " : ""
            );
        } else {
            if (!q) {
                q = true;
                fprintf(out, "b \"");
            }
            fprintf(out, "%c", string.buffer[i]);
        }
    }
    if (q) {
        fprintf(out, "\"");
    }
}

static void
qemit_data(struct qbe_definition definition[static 1], FILE out[static 1]) {
    assert(definition->definition_type == QBE_DEFINITION_TYPE_DATA);
    if (definition->data.section != nullptr
        && definition->data.section_flags != 0) {
        fprintf(
            out, "section \"%s\" \"%s\"", definition->data.section,
            definition->data.section_flags
        );
    } else if (definition->data.section != nullptr) {
        fprintf(out, "section \"%s\"", definition->data.section);
    } else if (is_zeroes(&definition->data.items)) {
        fprintf(out, "section \".bss.%s\"", definition->name.buffer);
    } else {
        fprintf(out, "section \".data.%s\"", definition->name.buffer);
    }
    fprintf(out, "\n");
    fprintf(out, "data $%s = ", definition->name.buffer);
    if (definition->data.align != ALIGN_UNDEFINED) {
        fprintf(out, "align %zu ", definition->data.align);
    }
    fprintf(out, "{ ");

    for (struct qbe_data_item* item = &definition->data.items; item != nullptr;
         item                       = item->next) {
        switch (item->data_type) {
            case QBE_DATA_TYPE_ZEROED:
                fprintf(out, "z %zu", item->zeroed);
                break;
            case QBE_DATA_TYPE_VALUE:
                emit_qtype(item->value.type, true, out);
                fprintf(out, " ");
                emit_value(&item->value, out);
                break;
            case QBE_DATA_TYPE_STRINGS:
                emit_data_string(item->string, out);
                break;
            case QBE_DATA_TYPE_SYMBOLS_OFFSETS:
                fprintf(out, "l $%s + %li", item->symbols, item->offsets);
                break;
        }

        fprintf(out, item->next != nullptr ? ", " : " ");
    }

    fprintf(out, "}\n\n");
}

static void
emit_definition(
    struct qbe_definition definition[static 1], FILE out[static 1]
) {
    switch (definition->definition_type) {
        case QBE_DEFINITION_TYPE_FUNCTION:
            break;
        case QBE_DEFINITION_TYPE_TYPE:
            qemit_type(definition, out);
            break;
        case QBE_DEFINITION_TYPE_DATA:
            qemit_data(definition, out);
            break;
    }
}

void
emit(struct qbe_program program[static 1], FILE out[static 1]) {
    struct qbe_definition* definition = program->definitions;
    while (definition) {
        emit_definition(definition, out);
        definition = definition->next;
    }
}
