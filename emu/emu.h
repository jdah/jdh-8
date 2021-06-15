#ifndef EMU_H
#define EMU_H

#include "../common/util.h"
#include "../common/jdh8.h"

#if defined(WIN32) || defined(_WIN32)
	#include "win/win.h"
#endif

// libemu.c
#define SIMULATE_ERROR         -1
#define SIMULATE_OK             0
#define SIMULATE_STOP_HALT      1
#define SIMULATE_STOP_MANUAL    2

int simulate(struct JDH8 *, u64, bool (*)(struct JDH8*));
void dump(FILE *f, struct JDH8 *state);

#define has_device(_state, _id) ((_state)->devices[_id].id != 0)
void add_device(struct JDH8*, struct Device);
void remove_device(struct JDH8*, u8 id);
void remove_devices(struct JDH8*);

void *add_bank(struct JDH8 *, u8);
void remove_bank(struct JDH8 *, u8);
void push(struct JDH8*, u8);
void push16(struct JDH8*, u16);
u8 pop(struct JDH8*);
u16 pop16(struct JDH8*);
void interrupt(struct JDH8*, u8);
u8 inb(struct JDH8*, u8);
void outb(struct JDH8*, u8, u8);
int load(struct JDH8* state, const char*, u16);
bool halted(struct JDH8 *state);
void step(struct JDH8*);

// mod.c
void mod_init();
usize mod_list(const char **names, usize n);
int mod_load(struct JDH8 *state, const char *name);

#endif
