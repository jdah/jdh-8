#include <unistd.h>
#if defined(WIN32) || defined(_WIN32)
	// Do nothing, emu.h already handles importing win.h in this case
#else
	#include <sys/mman.h>
#endif
#include <sys/types.h>
#include <pthread.h>

#include <SDL2/SDL.h>

#include "emu.h"

#define WINDOW_WIDTH    640
#define WINDOW_HEIGHT   480

#define SCREEN_WIDTH    160
#define SCREEN_HEIGHT   120

// shared global
static struct {
    u8 *bank;
    bool stop;
} screen;

static pthread_t child;
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;

static void screen_destroy(struct JDH8 *state, struct Device *dev) {
    screen.stop = true;
    pthread_join(child, NULL);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    remove_bank(state, 1);
}

static u8 screen_send(struct JDH8 *state, struct Device *dev) {
    return 0;
}

static void screen_receive(struct JDH8 *state, struct Device *dev, u8 data) {

}

static void screen_tick(struct JDH8 *state, struct Device *dev) {
    if (screen.stop) {
        remove_device(state, dev->id);
    } else {
        // SDL events must be polled on main thread
        for (SDL_Event e; SDL_PollEvent(&e);) {
            // defined in kb.c
            extern void kb_set_data(struct JDH8*, struct Device*, u8);

            switch (e.type) {
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    if (has_device(state, 3)) {
                        kb_set_data(
                            state, &state->devices[3],
                            (e.type == SDL_KEYUP ? 0x80 : 0x00) |
                                e.key.keysym.scancode
                        );
                    }
                    break;
                case SDL_QUIT:
                    screen.stop = true;
                    pthread_join(child, NULL);
                    remove_device(state, dev->id);
                    break;
                default:
                    break;
            }
        }
    }
}

static void fchild(struct Device *dev) {
    u32 data[SCREEN_HEIGHT][SCREEN_WIDTH];

    while (!screen.stop) {
        // convert memory bank into texture data
        for (usize y = 0; y < SCREEN_HEIGHT; y++) {
            for (usize x = 0; x < SCREEN_WIDTH; x++) {
                data[y][x] =
                    (((((u8 *) screen.bank)[(y * (SCREEN_WIDTH / 8)) + (x / 8)]) >>
                        (x % 8)) & 0x01) ?
                    0xFFFFFFFF : 0x00000000;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
        SDL_UpdateTexture(texture, NULL, data, SCREEN_WIDTH * 4);
        SDL_RenderCopy(
            renderer, texture,
            &((SDL_Rect) { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT }),
            &((SDL_Rect) { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT })
        );
        SDL_RenderPresent(renderer);
        SDL_UpdateWindowSurface(window);
    }
}

void screen_init(struct JDH8 *state, struct Device *dev) {
    *dev = (struct Device) {
        .id = 2,
        .send = screen_send,
        .receive = screen_receive,
        .tick = screen_tick,
        .destroy = screen_destroy
    };
    strncpy(dev->name, "SCREEN", sizeof(dev->name));

    screen.bank = add_bank(state, 1);
    screen.stop = false;
    memset(screen.bank, 0, 0x8000);

    assert(!SDL_Init(SDL_INIT_VIDEO));
    // TODO: remove windowpos
    window = SDL_CreateWindow(
        "JDH8",
        SDL_WINDOWPOS_CENTERED_DISPLAY(1),
        SDL_WINDOWPOS_CENTERED_DISPLAY(1),
        WINDOW_WIDTH, WINDOW_HEIGHT, 0
    );
    assert(window);
    renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    assert(renderer);
    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT
    );
    assert(texture);
    assert(!pthread_create(&child, NULL, (void* (*)(void*)) fchild, dev));
}
