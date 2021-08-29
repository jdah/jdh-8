#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>

#define F_CPU 16000000
#define BAUD 9600

#include <util/delay.h>
#include <util/setbaud.h>

// pins
#define PORT_VID    PORTD
#define DDR_VID     DDRD
#define PIN_VID     7

#define PORT_SYN   PORTB
#define DDR_SYN    DDRB
#define PIN_SYN    1

#define PORT_LED    PORTB
#define DDR_LED     DDRB
#define PIN_LED     5

#define NS_PER_TICK         250
#define NS_PER_US           1000
#define TICKS_PER_US        (NS_PER_US / NS_PER_TICK)

#define CYCLES_PER_SECOND   F_CPU
#define CYCLES_PER_MS       (CYCLES_PER_SECOND / 1000)
#define CYCLES_PER_US       (CYCLES_PER_MS / 1000)
#define CYCLES_PER_TICK     (CYCLES_PER_US / TICKS_PER_US)

#define NOP() _NOP()

#define DELAY_TCNT(n) do {                      \
        cli();                                  \
        uint16_t _dts = TCNT1;                  \
        while ((TCNT1 - _dts) < (n - 4)) { }    \
        sei();                                  \
    } while (0);

#define DELAY_CYCLE()   NOP()
#define DELAY_125NS()   DELAY_CYCLE(); DELAY_CYCLE()
#define DELAY_250NS()   DELAY_125NS(); DELAY_125NS()
#define DELAY_500NS()   DELAY_250NS(); DELAY_250NS()
#define DELAY_750NS()   DELAY_500NS(); DELAY_250NS()
#define DELAY_1000NS()  DELAY_500NS(); DELAY_500NS()
#define DELAY_1US()     DELAY_1000NS()
#define DELAY_2US()     DELAY_1US(); DELAY_1US()
#define DELAY_5US()     DELAY_2US(); DELAY_2US(); DELAY_1US()
#define DELAY_10US()    DELAY_5US(); DELAY_5US()
#define DELAY_20US()    DELAY_10US(); DELAY_10US()

static void seinit() {
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
    UCSR0B = _BV(TXEN0);
}

static void sewritec(char c) {
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
}

static void sewritestr(const char *str) {
    while (*str) {
        sewritec(*str++);
    }
}

static void seprintf(const char *fmt, ...) {
    char buf[512];
    va_list va_args;
    va_start(va_args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va_args);
    sewritestr(buf);
    va_end(va_args);
}

/*
 * NOTES:
 * - Takes 2 cycles to set a port
 * - One cycle @ 16 MHz = 62.5 ns (4 cycles = 250ns = 1 tick)
 */


int main(void) {
    seprintf("%s\r\n", "init again");
    seinit();

    // set timer1 to normal mode at F_CPU
    TCCR1A = 0;
    TCCR1B = 1;

    DDR_LED |= (1 << PIN_LED);
    DDR_VID |= (1 << PIN_VID);
    DDR_SYN |= (1 << PIN_SYN);

    volatile uint16_t hsc = 0;

    // 28 us @ 0.0v + 4 us @ 0.3v
#define SYN_SHORT_MINUS_2US()   \
    PORT_SYN &= ~(1 << PIN_SYN); DELAY_CYCLE(); \
    DELAY_20US();               \
    DELAY_5US();                \
    DELAY_2US();                \
    DELAY_750NS();              \
    DELAY_CYCLE();              \
    PORT_SYN |= (1 << PIN_SYN); \
    DELAY_1US();                \
    DELAY_750NS();              \
    DELAY_CYCLE();

#define SYN_SHORT()             \
    SYN_SHORT_MINUS_2US();      \
    DELAY_2US();

    // 2 us @ 0.0v + 30 us @ 0.3v
#define SYN_BROAD_MINUS_2US()   \
    PORT_SYN &= ~(1 << PIN_SYN); DELAY_CYCLE(); \
    DELAY_1US();                \
    DELAY_750NS();              \
    DELAY_CYCLE();              \
    PORT_SYN |= (1 << PIN_SYN); \
    DELAY_20US();               \
    DELAY_5US();                \
    DELAY_2US();                \
    DELAY_750NS();              \
    DELAY_CYCLE();

#define SYN_BROAD()             \
    SYN_BROAD_MINUS_2US();      \
    DELAY_2US();

    // 2 us = 32 cycles
#define VID_FP()                \
    PORT_SYN |= (1 << PIN_SYN);

    // 4 us = 64 cycles
#define VID_SYNC()              \
    PORT_SYN &= ~(1 << PIN_SYN);\
    DELAY_125NS();              \
    DELAY_750NS();              \
    DELAY_1US();                \
    DELAY_2US();

    // 6 us = 96 cycles
#define VID_BP()                \
    PORT_SYN |= (1 << PIN_SYN); \
    DELAY_CYCLE();              \
    DELAY_750NS();              \
    DELAY_5US();

#define VID_LINE(_op0, _op1)        \
    VID_SYNC();                     \
    VID_BP();                       \
    _op0;                           \
    DELAY_1US();                    \
    DELAY_750NS();                  \
    DELAY_20US();                   \
    DELAY_20US();                   \
    DELAY_10US();                   \
    _op1;                           \
    VID_FP();

    static const void *JMP_TABLE[640] = {
        [0   ...   4] = &&sig_eq,
        [5   ...   9] = &&sig_post,
        [10  ...  83] = &&sig_empty,
        [84  ... 563] = &&sig_data,
        [564 ... 617] = &&sig_empty,
        [618 ... 624] = &&sig_pre,
        [625 ... 639] = &&sig_reset
    };

    uint8_t data[26] = { [0 ... 25] = 0xEA };

#define PIXEL(_d, _j)                                       \
    PORT_VID = (_d & (1 << _j)) == 0 ?                      \
        ({ DELAY_CYCLE(); (PORT_VID & ~(1 << PIN_VID)); }) :\
        (PORT_VID | (1 << PIN_VID));

    // 1 us = 1000 ns = 16 cycles
    // total overhead from each jump is 32 cycles = 2 us
sig_reset:
    hsc = 0;
sig_eq:
    // 2560 cycles
    // 5 eq pulses
    SYN_SHORT();
    SYN_SHORT();
    SYN_SHORT();
    SYN_SHORT();
    SYN_SHORT_MINUS_2US();
    hsc += 5;
    goto *JMP_TABLE[hsc];
sig_post:
    // 2560 cycles
    // 5 post eq pulses
    SYN_BROAD();
    SYN_BROAD();
    SYN_BROAD();
    SYN_BROAD();
    SYN_BROAD_MINUS_2US();
    hsc += 5;
    goto *JMP_TABLE[hsc];
sig_empty:
    VID_LINE(
        PORT_VID &= ~(1 << PIN_VID); DELAY_CYCLE(),
        PORT_VID &= ~(1 << PIN_VID); DELAY_CYCLE()
    );
    hsc += 2;
    goto *JMP_TABLE[hsc];
sig_data:
    // data
    VID_SYNC();
    VID_BP();

    DELAY_1US();
    DELAY_2US();

    #pragma GCC unroll 26
    for (uint8_t i = 0; i < 26; i++) {
        uint8_t d = data[i];
        PORT_VID &= ~(1 << PIN_VID);

        #pragma GCC unroll 8
        for (uint8_t j = 0; j < 8; j++) {
            PIXEL(d, j);
            DELAY_CYCLE();
        }
    }
    PORT_VID &= ~(1 << PIN_VID); DELAY_CYCLE();
    DELAY_1US();
    DELAY_750NS();
    DELAY_CYCLE();
    VID_FP();
    hsc += 2;
    goto *JMP_TABLE[hsc];
sig_pre:
    // pre
    SYN_BROAD();
    SYN_BROAD();
    SYN_BROAD();
    SYN_BROAD();
    SYN_BROAD();
    SYN_SHORT_MINUS_2US();
    hsc += 6;
    goto *JMP_TABLE[hsc];
}
