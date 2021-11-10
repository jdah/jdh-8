#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

#if defined(WIN32) || defined(_WIN32)
    #include "win/win.h"
#else
    // POSIX specifics
    #include <poll.h>
    #include <readline/readline.h>
    #include <readline/history.h>
#endif

#include "emu.h"

// TODO: illegal instruction warnings

static bool stop_simulation(struct JDH8 *state) {
    struct pollfd pfds = { .fd = fileno(stdin), .events = POLLIN };

    if (poll(&pfds, 1, 0) > 0 && (pfds.events & POLLIN)) {
        char c;
        return read(fileno(stdin), &c, 1) > 0;
    }

    return false;
}

static int run(struct JDH8 *state, const char *speed, char *out, usize n) {
    // un-halt
    state->status = BIT_SET(state->status, S_HALT, 0);

    u64 hz = 16384;

    if (speed != NULL) {
        if (!strcasecmp(speed, "realtime")) {
            hz = 100000000; // 100 MHz (fastest possible)
        } else if (!strcasecmp(speed, "fast")) {
            hz = 2000000; // 2 MHz
        } else if (!strcasecmp(speed, "normal")) {
            hz = 1000000; // 1 MHz
        } else if (!strcasecmp(speed, "slow")) {
            hz = 8000; // 8 KHz
        } else if (!strcasecmp(speed, "veryslow")) {
            hz = 16; // 0.015 KHz
        } else if (strtou64(speed, 0, &hz)) {
            return -1;
        }
    }

    // TODO: bad
    fprintf(stdout, "Press enter to stop simulation\n");
    int res = simulate(state, hz, stop_simulation);

    switch (res) {
        case SIMULATE_ERROR:
            snprintf(out, n, "Error in simulation");
            break;
        case SIMULATE_OK:
            snprintf(out, n, "Done");
            break;
        case SIMULATE_STOP_HALT:
            snprintf(out, n, "Halted");
            break;
        case SIMULATE_STOP_MANUAL:
            snprintf(out, n, "Stopped");
            break;
        default:
            snprintf(out, n, "Unexpected simulation exit code");
            break;
    }

    return 0;
}

void command(struct JDH8 *state, const char *text, char *out, usize n) {
#define cmdchk(_e, _s) do {\
        if (!(_e)) {\
            strncpy(out, (_s), n);\
            return;\
        }\
    } while (0);

#define WRONG_ARGS      "Wrong number of arguments."
#define WRONG_NUMBER    "Malformatted/out of range number"

    char str[256], *token, *tokens[] = { NULL, NULL, NULL };
    usize num_tokens = 0;

    // default to returning nothing
    if (n > 0) {
        *out = 0;
    }

    if (strlen(text) > sizeof(str) - 1) {
        strncpy(out, "Illegal command (too long).", n);
        return;
    }

    strlcpy(str, text, sizeof(str));
    strupr(str);

    token = strtok(str, " ");
    while (token != NULL) {
        if (num_tokens >= 3) {
            strncpy(out, "Illegal command (too many tokens).", n);
            return;
        }

        tokens[num_tokens++] = token;
        token = strtok(NULL, " ");
    }

    if (num_tokens == 0) {
        strncpy(out, "Empty command", n);
    } else if (!strcmp("SET", tokens[0])) {
        cmdchk(num_tokens == 3, WRONG_ARGS);
        usize i;

        if ((i = strtab(R_NAMES, R_COUNT, tokens[1])) != (usize) -1) {
            u8 data;
            cmdchk(!strtou8(tokens[2], 0, &data), WRONG_NUMBER);
            state->registers.raw[i] = data;
        } else if ((i = strtab(SP_NAMES, SP_COUNT, tokens[1])) != (usize) -1) {
            u16 data;
            cmdchk(!strtou16(tokens[2], 0, &data), WRONG_NUMBER);
            state->special.raw[i] = data;
        } else {
            strncpy(out, "Unknown register", n);
        }
    } else if (!strcmp("GET", tokens[0])) {
        cmdchk(num_tokens == 2, WRONG_ARGS);
        usize i;

        if ((i = strtab(R_NAMES, R_COUNT, tokens[1])) != (usize) -1) {
            u8 data = state->registers.raw[i];
            snprintf(out, n, "0x%02x (%d)\n", data, data);
        } else if ((i = strtab(SP_NAMES, SP_COUNT, tokens[1])) != (usize) -1) {
            u16 data = state->special.raw[i];
            snprintf(out, n, "0x%04x (%d)\n", data, data);
        } else {
            strncpy(out, "Unknown register", n);
        }
    } else if (!strcmp("PEEK", tokens[0])) {
        cmdchk(num_tokens == 2, WRONG_ARGS);
        u16 addr;
        cmdchk(!strtou16(tokens[1], 0, &addr), WRONG_NUMBER);
        u8 data = peek(state, addr);
        snprintf(out, n, "0x%02x (%d)\n", data, data);
    } else if (!strcmp("POKE", tokens[0])) {
        cmdchk(num_tokens == 3, WRONG_ARGS);
        u16 addr;
        cmdchk(!strtou16(tokens[1], 0, &addr), WRONG_NUMBER);
        u8 data;
        cmdchk(!strtou8(tokens[2], 0, &data), WRONG_NUMBER);
        poke(state, addr, data);
    } else if (!strcmp("INB", tokens[0])) {
        cmdchk(num_tokens == 2, WRONG_ARGS);
        u8 port;
        cmdchk(!strtou8(tokens[1], 0, &port), WRONG_NUMBER);
        u8 data = inb(state, port);
        snprintf(out, n, "0x%02x (%d)\n", data, data);
    } else if (!strcmp("OUTB", tokens[0])) {
        cmdchk(num_tokens == 3, WRONG_ARGS);
        u8 port, data;
        cmdchk(!strtou8(tokens[1], 0, &port), WRONG_NUMBER);
        cmdchk(!strtou8(tokens[2], 0, &data), WRONG_NUMBER);
        outb(state, port, data);
    } else if (!strcmp("DUMP", tokens[0])) {
        cmdchk(num_tokens == 1, WRONG_ARGS);
        FILE *f = fmemopen((void *) out, n, "wb");
        assert(f != NULL);
        dump(f, state);
        assert(!fclose(f));
    } else if (!strcmp("STEP", tokens[0])) {
        cmdchk(num_tokens == 1, WRONG_ARGS);
        step(state);
    } else if (!strcmp("LOAD", tokens[0])) {
        cmdchk(num_tokens != 1, WRONG_ARGS);
        u16 addr = 0;

        if (num_tokens == 3) {
            cmdchk(!strtou16(tokens[2], 0, &addr), WRONG_NUMBER);
        }

        cmdchk(!load(state, tokens[1], addr), "Error loading file");
    } else if (!strcmp("QUIT", tokens[0])) {
        cmdchk(num_tokens == 1, WRONG_ARGS);
        exit(0);
    } else if (!strcmp("MODS", tokens[0])) {
        cmdchk(num_tokens == 1, WRONG_ARGS);

        char buf[4096];
        buf[0] = '\0';

        const char *names[256];
        usize num = mod_list(names, ARRLEN(names));

        if (num == 0) {
            snprintf(out, n, "%s\n", "No modules");
        } else {
            str[0] = '\0';
            for (usize i = 0; i < num; i++) {
                if (i != 0) {
                    strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
                }

                strncat(buf, names[i], sizeof(buf) - strlen(buf) - 1);
            }

            snprintf(out, n, "Modules: %s\n", buf);
        }
    } else if (!strcmp("MOD", tokens[0])) {
        cmdchk(num_tokens == 2, WRONG_ARGS);
        cmdchk(!mod_load(state, tokens[1]), "Error loading module");
    } else if (!strcmp("DEVICES", tokens[0])) {
        cmdchk(num_tokens == 1, WRONG_ARGS);

        for (usize i = 0; i < DEVICE_MAX; i++) {
            struct Device *dev = &state->devices[i];

            if (dev->id == 0) {
                continue;
            }

            snprintf(
                out + strlen(out),
                n - strlen(out),
                "[%d] %s\n",
                dev->id, dev->name
            );
            dev++;
        }
    } else if (!strcmp("RUN", tokens[0])) {
        cmdchk(num_tokens == 1 || num_tokens == 2, WRONG_ARGS);
        cmdchk(
            !run(state, num_tokens == 2 ? tokens[1] : NULL, out, n),
            "Invalid run speed"
        );
    } else {
        snprintf(out, n, "Unrecognized command %s\n", tokens[0]);
    }
}

int main(int argc, const char *argv[]) {
    struct JDH8 state;
    memset(&state, 0, sizeof(state));
    mod_init();

    bool loaded = false;
    const char *run_speed = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--mod")) {
            assert(i + 1 != argc);
            mod_load(&state, argv[++i]);
        } else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--run")) {
            assert(i + 1 != argc);
            run_speed = argv[++i];
        } else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--load")) {
            assert(i + 2 < argc);
            u16 addr;
            assert(!strtou16(argv[i + 2], 0, &addr));
            fprintf(stdout, "Loading ROM from %s at 0x%04x..\n", argv[i + 1], addr);
            load(&state, argv[i + 1], addr);
            loaded |= true;
            i += 2;
        } else {
            fprintf(stdout, "Loading ROM from %s...\n", argv[1]);
            load(&state, argv[1], 0);
            loaded |= true;
        }
    }

    if (!loaded) {
        fprintf(stdout, "%s\n", "No ROM loaded");
    }

    while (true) {
        char out[8192];

        if (run_speed) {
            run(&state, run_speed, out, sizeof(out));
            run_speed = NULL;
        } else {
            char *buf = readline(">");

            // EOF (^D)
            if (!buf) {
                exit(0);
            }

            buf[strcspn(buf, "\n")] = 0;
            add_history(buf);

            out[0] = '\0';
            command(&state, buf, out, sizeof(out));

            usize outlen = strlen(out);
            if (outlen != 0 && out[outlen - 1] != '\n') {
                out[outlen] = '\n';
                out[outlen + 1] = '\0';
            }

            fprintf(stdout, "%s", out);
            free(buf);
        }
    }

    return 0;
}
