#include "emu.h"

static u8 data;

static u8 kb_send(struct JDH8 *state, struct Device *dev) {
    return data;
}

// called from screen.c
void kb_set_data(struct JDH8 *state, struct Device *dev, u8 d)  {
    data = d;
}

void kb_init(struct JDH8 *state, struct Device *dev) {
    *dev = (struct Device) {
        .id = 3,
        .send = kb_send,
        .receive = NULL,
        .tick = NULL,
        .destroy = NULL
    };
    strncpy(dev->name, "KEYBOARD", sizeof(dev->name));
}
