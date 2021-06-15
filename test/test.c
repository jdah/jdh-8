#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../common/util.h"
#include "../common/jdh8.h"
#include "../emu/emu.h"

static const char *asmpath, *testpath;

static bool verbose = false;

struct Assertion {
    bool reg;
    u8 value;

    union {
        char name;
        u16 addr;
    };
};

static void dbg(const char *fmt, ...) {
    if (!verbose) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    printf("%s", "[DEBUG] ");
    vfprintf(stdout, fmt, args);
    va_end(args);
}

static bool test(const char *filename) {
    printf("Testing file %s ... ", filename);

    // assertions stated in test file
    usize num_assertions = 0;
    struct Assertion assertions[32];

    // read file contents
    FILE *f = fopen(filename, "rb");
    assert(f);

    fseek(f, 0, SEEK_END);
    usize len = ftell(f);
    rewind(f);

    char *buf = malloc(len + 1);
    assert(buf);
    assert(fread((void *) buf, len, 1, f) == 1);
    fclose(f);
    buf[len] = '\0';

    dbg("Read %d bytes from file %s\n", len, filename);

    // parse first lines, verify formatting
    bool test_found = false;

    char *str = buf;
    while (*str != '\0' &&
            *(str + 1) != '\0' &&
            !(str != buf && *(str - 1) == '\n' && *str == '\n')) {
        char *eol = strchr(str, '\n');
        assert(eol);

        // seek to first non-space
        while (str != eol && isspace(*str))  {
            str++;
        }

        // must be comment
        assert(*str == ';');

        if (!test_found) {
            // first line must be "; TEST"
            assert(strstr(str, "; TEST\n") == str);
            test_found = true;
        } else {
            // skip ;
            str++;

            // other lines must assert about assembler state
            char *eq = strchr(str, '=');
            assert(eq && eq < eol);

            while (isspace(*str)) {
                str++;
            }

            if (*str == '[') {
                // memory address
                char *rb = strchr(str, ']');
                assert(rb && rb < eq);

                char nstr[16];
                assert((usize) (rb - str) <= sizeof(nstr));
                memcpy(nstr, str + 1, rb - str - 1);
                nstr[rb - str - 1] = '\0';

                assert(!strtou16(nstr, 0, &assertions[num_assertions].addr));
                assertions[num_assertions++].reg = false;
                str = rb + 1;
            } else {
                // register
                assertions[num_assertions++] = (struct Assertion) {
                    .reg = true,
                    .name = *str
                };

                str++;
            }

            while (str != eq) {
                assert(isspace(*str));
                str++;
            }

            str++;

            char value[256];
            memcpy(value, str, eol - str);
            value[eol - str] = '\0';
            strstrip(value);

            i64 v;
            assert(!strtoi64(value, 0, &v));
            assertions[num_assertions - 1].value = v;
        }

        str = eol + 1;
    }

    char command[256];

    // generate temporary filename
    // TODO: better way to do this without deleting?
    char tmpfile[] = "/tmp/jdh8-asmtest-XXXXXXXX";
    assert(mkstemp(tmpfile) != -1);
    dbg("tmpfile is %s\n", tmpfile);

    strncpy(command, "rm -f ", sizeof(command));
    strncat(command, tmpfile, sizeof(command) - strlen(command) - 1);
    assert(!system(command));
    dbg("Running \"%s\"\n", command);

    // assemble the file
    strncpy(command, asmpath, sizeof(command));
    strncat(command, " -o ", sizeof(command) - strlen(command) - 1);
    strncat(command, tmpfile, sizeof(command) - strlen(command) - 1);
    strncat(command, " ", sizeof(command) - strlen(command) - 1);
    strncat(command, filename, sizeof(command) - strlen(command) - 1);
    assert(!system(command));
    dbg("Running \"%s\"\n", command);

    // emulate until halt
    struct JDH8 state;
    memset(&state, 0, sizeof(struct JDH8));

    dbg("Emulating...\n");
    load(&state, tmpfile, 0);
    usize count = 0;
    while (!halted(&state)) {
        step(&state);

        if (++count >= 65536) {
            printf(
                ANSI_MAGENTA "%s\n" ANSI_RESET,
                "aborted"
            );
            return false;
        }
    }
    dbg("Emulation complete, CPU has halted\n");

    // check assertions
    for (usize i = 0; i < num_assertions; i++) {
        struct Assertion *a = &assertions[i];

        char src[256];
        snprintf(
            src, sizeof(src),
            a->reg ? "%c" : "[0x%04X]",
            a->reg ? a->name : a->addr
        );

        u8 value;

        if (a->reg) {
            char name[2];
            name[0] = a->name;
            name[1] = '\0';

            isize i = -1;
            assert((i = strcasetab(R_NAMES, R_COUNT, name)) != -1);
            value = state.registers.raw[i];
        } else {
            value = peek(&state, a->addr);
        }

        if (value != a->value) {
            printf(
                ANSI_RED "expected %s = 0x%02X, got 0x%02X\n" ANSI_RESET,
                src, a->value, value
            );
            return false;
        }

        dbg("Assertion %s = 0x%02X passed", src, a->value);
    }

    printf(ANSI_GREEN "%s" ANSI_RESET "\n", "OK");
    return true;
}

static void usage() {
    printf(
        "usage: test [-a/--asmpath path] [-v]"
        " [--verbose] [test file or directory]\n");
}

int main(int argc, const char *argv[]) {
	asmpath = "./bin/asm";
	testpath = "./test";

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") ||
                !strcmp(argv[i], "--help")) {
            usage();
            exit(0);
        } else if (!strcmp(argv[i], "-a") ||
                !strcmp(argv[i], "--asmpath")) {
            assert(i != argc - 1);
            asmpath = argv[++i];
        } else if (!strcmp(argv[i], "-v") ||
                !strcmp(argv[i], "--verbose")) {
            verbose = true;
        } else {
            testpath = argv[i];
        }
    }

    assert(!access(asmpath, F_OK));
    assert(!access(testpath, F_OK | R_OK));

    struct stat sb;
    assert(!stat(testpath, &sb));
    if (S_ISDIR(sb.st_mode)) {
        // find all files with .asm extension
        DIR *d;
        assert(d = opendir(testpath));

        struct dirent *e;
        while ((e = readdir(d))) {
            // construct relative path
            char path[256];
            strncpy(path, testpath, sizeof(path));
            strncat(path, "/", sizeof(path) - strlen(path) - 1);
            strncat(path, e->d_name, sizeof(path) - strlen(path) - 1);

            assert(!stat(path, &sb));

            if (e->d_name[0] == '.') {
                // hidden file
            } else if (S_ISDIR(sb.st_mode)) {
                printf("Skipping directory %s\n", path);
            } else if (strsuf(".asm", e->d_name)) {
                test(path);
            } else {
                printf("Skipping file %s\n", path);
            }
        }

        closedir(d);
    } else {
        test(testpath);
    }
}
