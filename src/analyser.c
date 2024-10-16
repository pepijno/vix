#include "analyser.h"

#include <stdlib.h>
#include <string.h>

struct function_type function_types[1024];

struct nearest_object {
    i32 id;
    struct ast_free_property* free_prop;
    struct ast_object* object;
};

static struct nearest_object
find_nearest_object(struct ast_object object[static 1], char* name) {
    struct ast_free_property* free_prop = object->free_properties;
    while (free_prop != nullptr) {
        if (strcmp(free_prop->name, name) == 0) {
            return (struct nearest_object){
                .id        = free_prop->id,
                .free_prop = free_prop,
                .object    = nullptr,
            };
        }
        free_prop = free_prop->next;
    }
    if (object->type == OBJECT_TYPE_PROPERTIES) {
        struct ast_property* prop = object->properties;
        while (prop != nullptr) {
            if (strcmp(prop->name, name) == 0) {
                return (struct nearest_object){
                    .id        = prop->object->id,
                    .free_prop = nullptr,
                    .object    = prop->object,
                };
            }
            prop = prop->next;
        }
    }
    if (object->id == 0) {
        vix_panic("bla");
    }
    return find_nearest_object(object->parent, name);
}

static struct function_type*
update_types(struct ast_object object[static 1]) {
    struct function_type* type = &function_types[object->id];

    type->param_types = calloc(1, sizeof(struct function_param_type));
    struct function_param_type** next   = &type->param_types;
    struct ast_free_property* free_prop = object->free_properties;
    while (free_prop != nullptr) {
        (*next)->name = malloc(sizeof(char) * (strlen(free_prop->name) + 1));
        strcpy((*next)->name, free_prop->name);
        (*next)->next          = calloc(1, sizeof(struct function_param_type));
        (*next)->function_type = &function_types[free_prop->id];
        (*next)->function_type->id                      = free_prop->id;
        (*next)->function_type->return_type.return_type = RETURN_ANY;
        next                                            = &(*next)->next;
        free_prop                                       = free_prop->next;
    }
    free(*next);
    *next = nullptr;

    switch (object->type) {
        case OBJECT_TYPE_NONE:
            break;
        case OBJECT_TYPE_OBJECT_COPY: {
            type->return_type.return_type = RETURN_COPY;
            struct ast_object_copy* copy  = object->object_copy;
            struct nearest_object nearest_object
                = find_nearest_object(object, copy->name);
            i32 id                      = nearest_object.id;
            type->return_type.copy_type = &function_types[id];

            if (nearest_object.object != nullptr
                && function_types[id].id == -1) {
                update_types(nearest_object.object);
            } else if (nearest_object.free_prop != nullptr
                       && function_types[id].id == -1) {
                function_types[id].id                      = free_prop->id;
                function_types[id].return_type.return_type = RETURN_ANY;
            }

            struct ast_free_property_assign* free_assign
                = copy->free_properties;
            struct function_param_type* param_type
                = type->return_type.copy_type->param_types;
            while (free_assign != nullptr) {
                if (function_types[free_assign->value->id].id == -1) {
                    update_types(free_assign->value);
                }
                if (param_type->function_type->return_type.return_type
                    == RETURN_ANY) {
                    param_type->function_type->return_type.return_type
                        = RETURN_UNION;
                    param_type->function_type->return_type.function_union
                        = calloc(1, sizeof(struct function_union));
                    param_type->function_type->return_type.function_union->type
                        = &function_types[free_assign->value->id];
                    param_type->function_type->return_type.function_union->next
                        = nullptr;
                } else if (param_type->function_type->return_type.return_type
                           == RETURN_UNION) {
                    struct function_union* last
                        = param_type->function_type->return_type.function_union;
                    while (last != nullptr) {
                        if (last->next != nullptr) {
                            last = last->next;
                        } else {
                            break;
                        }
                    }
                    last->next       = calloc(1, sizeof(struct function_union));
                    last->next->type = &function_types[free_assign->value->id];
                    last->next->next = nullptr;
                } else {
                    vix_panic("blabla");
                }

                param_type  = param_type->next;
                free_assign = free_assign->next;
            }

            break;
        }
        case OBJECT_TYPE_PROPERTIES: {
            type->return_type.return_type = RETURN_OBJECT;
            type->return_type.properties
                = calloc(1, sizeof(struct return_object_property));
            struct return_object_property** next_prop
                = &type->return_type.properties;

            struct ast_property* prop = object->properties;
            while (prop != nullptr) {
                (*next_prop)->name
                    = malloc(sizeof(char) * (strlen(prop->name) + 1));
                strcpy((*next_prop)->name, prop->name);

                (*next_prop)->function_type = update_types(prop->object);

                (*next_prop)->next
                    = calloc(1, sizeof(struct return_object_property));
                next_prop = &(*next_prop)->next;
                prop      = prop->next;
            }

            free(*next_prop);
            *next_prop = nullptr;

            break;
        }
        case OBJECT_TYPE_INTEGER: {
            type->return_type.return_type = RETURN_INTEGER;
            break;
        }
        case OBJECT_TYPE_STRING: {
            type->return_type.return_type = RETURN_STRING;
            break;
        }
    }

    return type;
}

char*
type_to_string(enum return_type type) {
    switch (type) {
        case RETURN_INTEGER:
            return "INTEGER";
        case RETURN_STRING:
            return "STRING";
        case RETURN_OBJECT:
            return "OBJECT";
        case RETURN_COPY:
            return "COPY";
        case RETURN_UNION:
            return "UNION";
        case RETURN_ANY:
        default:
            return "ANY";
    }
}

void
print_types(struct function_type type[static 1], u8 indent) {
    if (type->param_types != nullptr) {
        printf("%*sfrees:\n", indent, "");
        struct function_param_type* param_type = type->param_types;
        while (param_type != nullptr) {
            printf("%*s  %s:\n", indent, "", param_type->name);
            print_types(param_type->function_type, indent + 4);
            param_type = param_type->next;
        }
    }

    printf(
        "%*stype %s\n", indent, "",
        type_to_string(type->return_type.return_type)
    );
    if (type->return_type.return_type == RETURN_OBJECT) {
        struct return_object_property* prop = type->return_type.properties;
        while (prop != nullptr) {
            printf("%*s  %s:\n", indent, "", prop->name);
            print_types(prop->function_type, indent + 4);
            prop = prop->next;
        }
    } else if (type->return_type.return_type == RETURN_COPY) {
        printf("%*scopy:\n", indent, "");
        print_types(type->return_type.copy_type, indent + 2);
    } else if (type->return_type.return_type == RETURN_UNION) {
        printf("%*sunion:\n", indent, "");
        struct function_union* function_union
            = type->return_type.function_union;
        while (function_union != nullptr) {
            print_types(function_union->type, indent + 2);
            function_union = function_union->next;
        }
    }
}

void
analyse(struct ast_object root[static 1]) {
    for (size i = 0; i < 1024; ++i) {
        function_types[i].id = -1;
    }

    struct function_type* root_type = update_types(root);
    print_types(root_type, 0);
}
