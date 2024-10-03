#include "analyser.h"

static void
find_usages(char const* name, struct ast_object object[static 1]) {
    name = name;
    switch (object->type) {
        case OBJECT_TYPE_OBJECT_COPY:
            break;
        case OBJECT_TYPE_PROPERTIES:
            break;
        case OBJECT_TYPE_NONE:
            break;
        case OBJECT_TYPE_INTEGER:
            break;
        case OBJECT_TYPE_STRING:
            break;
    }
}

static void
find_functions(struct ast_object root[static 1], char const* name, struct ast_object object[static 1]) {
    if (object->properties != nullptr) {
        find_usages(name, root);
    }

    switch (object->type) {
        case OBJECT_TYPE_NONE:
            break;
        case OBJECT_TYPE_OBJECT_COPY:
            break;
        case OBJECT_TYPE_PROPERTIES: {
            struct ast_property* prop = object->properties;
            while (prop != nullptr) {
                find_functions(root, prop->name, prop->object);
                prop = prop->next;
            }
            break;
        }
        case OBJECT_TYPE_INTEGER:
            break;
        case OBJECT_TYPE_STRING:
            break;
    }
}

void
analyse(struct ast_object root[static 1]) {
    find_functions(root, "root", root);
}
