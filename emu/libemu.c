#include <stdint.h>
#include <time.h>

#include "emu.h"

#define NS_PER_SECOND 1000000000

#if defined(WIN32) || defined(_WIN32)
	#define MONO_CLOCK CLOCK_MONOTONIC
#else
	#define MONO_CLOCK _CLOCK_MONOTONIC_RAW
#endif

#define NOW() __extension__({                               \
        struct timespec ts;                                 \
        assert(!clock_gettime(MONO_CLOCK, &ts));            \
        ((ts.tv_sec * NS_PER_SECOND) + ts.tv_nsec);         \
    })

#define SLEEP(ns) __extension__({                           \
        const u64 n = (ns);                                 \
        if (n > 0) {                                        \
            struct timespec req, rem;                       \
            req.tv_sec = 0;                                 \
            req.tv_nsec = ns;                               \
            nanosleep(&req, &rem);                          \
            ((rem.tv_sec * NS_PER_SECOND) + rem.tv_nsec);   \
        } else { 0; }                                       \
    })

// max speed 128 mhz
#define SIMULATE_MAX_SPEED 1000000000

#define SIMULATE_TICK_NS (NS_PER_SECOND / 10000)

int simulate(struct JDH8 *state, u64 hz, bool (*stop)(struct JDH8*)) {
    if (hz > SIMULATE_MAX_SPEED) {
        return SIMULATE_ERROR;
    }

    int result = SIMULATE_OK;
    state->simulating = true;

    const u64 OPS_PER_STEP = hz < 512 ? 1 : 512;
    const u64 ns_per_step = NS_PER_SECOND / (hz / OPS_PER_STEP);
    u64 start = NOW(),
        check = start,
        last = start,
        now = start;

    while (true) {
        now = NOW();

        if (now - check >= SIMULATE_TICK_NS) {
            FOR_DEVICES(state, id, dev) {
                if (dev->id == 0 || !dev->tick) {
                    continue;
                }

                dev->tick(state, dev);
            }

            check = now;
        }

        if (now - last >= ns_per_step) {
            for (usize i = 0; i < OPS_PER_STEP && !halted(state); i++) {
                step(state);
            }
            last = now;
        }

        if (stop(state)) {
            result = SIMULATE_STOP_MANUAL;
            goto done;
        }

        if (halted(state)) {
            result = SIMULATE_STOP_HALT;
            goto done;
        }

        u64 sleep_time = ns_per_step - (NOW() - now);
        if (sleep_time > NS_PER_SECOND) {
            sleep_time = 0;
        }
        SLEEP(sleep_time);
    }

done:
    state->simulating = false;
    return result;
}

void dump(FILE *f, struct JDH8 *state) {
    fprintf(
        f, "MB: 0x%04x PC: 0x%04x SP: 0x%04x\n",
        state->special.mb,
        state->special.pc,
        state->special.sp
    );

    for (usize i = 0; i < R_COUNT; i++) {
        fprintf(f, "%s: 0x%02x\n", R_NAME(i), state->registers.raw[i]);
    }

    for (usize i = 0; i < F_COUNT; i++) {
        fprintf(
            f,
            "  %s: %s\n",
            F_NAME(i),
            (state->registers.f & (1 << i)) ? "true" : "false"
        );
    }

    for (usize i = 0; i < SP_COUNT; i++) {
        fprintf(f, "%s: 0x%04x\n", SP_NAME(i), state->special.raw[i]);
    }

    fprintf(f, "STATUS: 0x%02x\n", state->status);
    for (usize i = 0; i < S_COUNT; i++) {
        fprintf(
            f,
            "  %s: %s\n",
            S_NAME(i),
            (state->status & (1 << i)) ? "true" : "false"
        );
    }

    fprintf(f, "%s", "DEVICES\n");
    FOR_DEVICES(state, id, dev) {
        if (dev->id == 0) {
            continue;
        }

        fprintf(
            f,
            "  (%" PRIu64 ") %s\n",
            id, dev->name
        );
    }
}

void add_device(struct JDH8 *state, struct Device device) {
    assert(state->devices[device.id].id == 0);
    memcpy(&state->devices[device.id], &device, sizeof(struct Device));
}

void remove_device(struct JDH8 *state, u8 id) {
    assert(state->devices[id].id != 0);
    struct Device *dev = &state->devices[id];

    if (dev->destroy) {
        dev->destroy(state, dev);
    }

    if (dev->state) {
        free(dev->state);
    }

    dev->id = 0;
}

void remove_devices(struct JDH8 *state) {
    FOR_DEVICES(state, id, dev) {
        if (dev->id != 0) {
            remove_device(state, dev->id);
        }
    }
}

void *add_bank(struct JDH8 *state, u8 i) {
    assert(i != 0);
    assert(!state->banks[i]);

    void *res = malloc(0x8000);
    assert(res);
    state->banks[i] = res;
    return res;
}

void remove_bank(struct JDH8 *state, u8 i) {
    assert(i != 0);
    assert(state->banks[i]);
    free(state->banks[i]);
    state->banks[i] = NULL;
}

void push(struct JDH8 *state, u8 data) {
    poke(state, state->special.pc--, data);
}

void push16(struct JDH8 *state, u16 data) {
    poke(state, state->special.pc--, (data >> 8) & 0xFF);
    poke(state, state->special.pc--, (data >> 0) & 0xFF);
}

u8 pop(struct JDH8 *state) {
    return peek(state, ++state->special.pc);
}

u16 pop16(struct JDH8 *state) {
    return
        (((u16) peek(state, ++state->special.pc)) << 0) |
        (((u16) peek(state, ++state->special.pc)) << 8);
}

void interrupt(struct JDH8 *state, u8 n) {
    assert(n <= 125);
    push16(state, state->special.pc);
    state->special.pc = peek16(state, ADDR_INTERRUPT_TABLE + (n * 2));
    state->status = BIT_SET(state->status, S_INTERRUPT, 1);
}

u8 inb(struct JDH8 *state, u8 port) {
    struct Device *dev;

    switch (port) {
        case P_STATUS_REGISTER:
            return state->status;
        case P_INTERRUPT_CONTROL:
            warn("Read from interrupt control\n");
            break;
        default:
            dev = &state->devices[port];
            if (dev->id != 0) {
                if (!dev->send) {
                    warn("inb from device on port %u with no send()\n", port);
                    return 0;
                } else {
                    return dev->send(state, dev);
                }
            }

            warn("Unused port for INB: %u\n", port);
    }

    return 0;
}

void outb(struct JDH8 *state, u8 port, u8 data) {
    switch (port) {
        case P_STATUS_REGISTER:
            if ((data & (1 << S_INTERRUPT)) !=
                    (state->status & (1 << S_INTERRUPT))) {
                warn("Illegal attempted modification of STATUS:INTERRUPT");
            }
            state->status = data;
            break;
        case P_INTERRUPT_CONTROL:
            if (data == 0xFF) {
                if (state->status & (1 << S_INTERRUPT)) {
                    state->special.pc = pop16(state);
                } else {
                    warn("Illegal attempted interrupt return\n");
                }
            } else {
                interrupt(state, data);
            }
            break;
        default:
            if (state->devices[port].id != 0) {
                state->devices[port].receive(
                    state,
                    &state->devices[port],
                    data
                );
            } else {
                warn("Unused port for OUTB: %u (data=%u)\n", port, data);
            }
    }

}

int load(struct JDH8* state, const char *filename, u16 addr) {
    FILE *f = fopen(filename, "rb");

    if (f == NULL) {
        return -1;
    }

    fseek(f, 0, SEEK_END);
    usize len = ftell(f);
    rewind(f);

    if (addr + len > UINT16_MAX) {
        warn(
            "File too big, will overwrite %" PRIu64 " bytes starting from 0",
            (addr + len) % UINT16_MAX);
    }

    u8 *buf = malloc(len);
    if (!buf || fread(buf, len, 1, f) != 1) {
        return -1;
    }
    emu_memcpy(state, addr, buf, len);
    fclose(f);
    free(buf);
    return 0;
}

bool halted(struct JDH8 *state) {
    return state->status & (1 << S_HALT);
}

void step(struct JDH8 *state) {
    if (halted(state)) {
        return;
    }

    u8 pc0 = peek(state, state->special.pc++), pc1, a, b, r, f;
    enum Instruction ins = (pc0 & 0xF0) >> 4;
    u16 hl;

#define IMM8_REG() __extension__({                          \
        pc1 = peek(state, state->special.pc++);             \
        (pc0 & 0x8) ? state->registers.raw[pc1 & 0x7] : pc1;\
    })

#define IMM8_PC0_REG()                      \
    ((pc0 & 0x08) ?                         \
        state->registers.raw[pc0 & 0x7] :   \
        peek(state, state->special.pc++))

#define IMM16() (                                   \
    ((u16) peek(state, state->special.pc++)) |      \
    (((u16) peek(state, state->special.pc++)) << 8))\

    switch (ins) {
        case I_MW:
            state->registers.raw[pc0 & 0x7] = IMM8_REG();
            break;
        case I_LW:
            state->registers.raw[pc0 & 0x7] = peek(
                state,
                pc0 & 0x8 ? state->registers.hl : IMM16()
            );
            break;
        case I_SW:
            poke(
                state,
                pc0 & 0x8 ? state->registers.hl : IMM16(),
                state->registers.raw[pc0 & 0x7]
            );
            break;
        case I_PUSH:
            poke(state, state->special.sp--, IMM8_PC0_REG());
            break;
        case I_POP:
            state->registers.raw[pc0 & 0x7] = peek(state, ++state->special.sp);
            break;
        case I_LDA:
            hl = IMM16();
            state->registers.hl = hl;
            break;
        case I_JNZ:
            if (IMM8_PC0_REG() != 0) {
                state->special.pc = state->registers.hl;
            }
            break;
        case I_INB:
            state->registers.raw[pc0 & 0x7] = inb(state, IMM8_REG());
            break;
        case I_OUTB:
            outb(state, IMM8_REG(), state->registers.raw[pc0 & 0x7]);
            break;
        case I_ADD:
        case I_ADC:
        case I_AND:
        case I_OR:
        case I_NOR:
        case I_CMP:
        case I_SBB:
            a = state->registers.raw[pc0 & 0x7];
            b = IMM8_REG();
            r = 0;

            switch (ins) {
                case I_CMP: r = a; break;
                case I_AND: r = a & b; break;
                case I_OR:  r = a | b; break;
                case I_NOR: r = ~(a | b); break;
                case I_ADC:
                    r += !!(state->registers.f & (1 << F_CARRY));
                case I_ADD:
                    r += a + b;
                    break;
                case I_SBB:
                    r = a - b - !!(state->registers.f & (1 << F_BORROW));
                    break;
                default: break;
            }

            f = state->registers.f;
            state->registers.raw[pc0 & 0x7] = r;

            // load flags
            if (ins == I_ADD ||
                    ins == I_ADC ||
                    ins == I_SBB ||
                    ins == I_CMP) {
                f = BIT_SET(f, F_LESS, a < b);
                f = BIT_SET(f, F_EQUAL, a == b);
                f = BIT_SET(
                    f, F_CARRY,
                    (ins == I_ADD || ins == I_ADC) ?
                        ((((u16) a) + ((u16) (b))) > 255 ? 1 : 0) :
                        !!(f & (1 << F_CARRY))
                );
                f = BIT_SET(
                    f, F_BORROW,
                    ins == I_SBB ?
                        (b > a ? 1 : 0) :
                        !!(f & (1 << F_BORROW))
                );
                state->registers.f = f;
            }

            break;
    }

    if (!state->simulating) {
        FOR_DEVICES(state, id, dev) {
            if (dev->id == 0 || !dev->tick) {
                continue;
            }

            dev->tick(state, dev);
        }
    }
}

