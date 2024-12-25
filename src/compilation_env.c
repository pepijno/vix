#include "compilation_env.h"

#include "util.h"

usize
get_offset(struct compilation_env compilation_env, struct string name) {
    switch (compilation_env.type) {
        case COMPILATION_ENV_TYPE_VAR:
            if (strings_equal(compilation_env.env_var.name, name)) {
                return 0;
            } else if (compilation_env.parent != nullptr) {
                return get_offset(*compilation_env.parent, name) + 1;
            } else {
                vix_unreachable();
            }
        case COMPILATION_ENV_TYPE_OFFSET:
            if (compilation_env.parent != nullptr) {
                return get_offset(*compilation_env.parent, name)
                     + compilation_env.env_offset.offset;
            } else {
                vix_unreachable();
            }
    }
    vix_unreachable();
}

bool
has_variable(struct compilation_env compilation_env, struct string name) {
    switch (compilation_env.type) {
        case COMPILATION_ENV_TYPE_VAR:
            if (strings_equal(compilation_env.env_var.name, name)) {
                return true;
            } else if (compilation_env.parent != nullptr) {
                return has_variable(*compilation_env.parent, name);
            } else {
                return false;
            }
        case COMPILATION_ENV_TYPE_OFFSET:
            if (compilation_env.parent != nullptr) {
                return has_variable(*compilation_env.parent, name);
            } else {
                return false;
            }
    }
    return false;
}
