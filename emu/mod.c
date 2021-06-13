#include "emu.h"

struct Module {
    const char *name;
    void (*init)(struct JDH8*, struct Device*);
};

#define MODULES_MAX 128
static usize num_modules = 0;
static struct Module modules[MODULES_MAX];

#define MOD_DECL(_name) do {                                    \
        extern void _name##_init(struct JDH8 *, struct Device*);\
        modules[num_modules++] = (struct Module) {              \
            .name = #_name,                                     \
            .init = _name##_init                                \
        };                                                      \
    } while (0);

void mod_init() {
    MOD_DECL(screen);
    MOD_DECL(kb);
}

usize mod_list(const char **names, usize n) {
    usize i = 0;
    for (; i < num_modules && i < n; i++) {
        names[i] = modules[i].name;
    }
    return i;
}

int mod_load(struct JDH8 *state, const char *name) {
    usize i;
    struct Module *mod;

    for (i = 0; i < num_modules; i++) {
        mod = &modules[i];

        if (!strcasecmp(name, mod->name)) {
            break;
        }
    }

    if (i == num_modules) {
        return -1;
    }

    struct Device dev;
    mod->init(state, &dev);
    add_device(state, dev);
    return 0;
}

