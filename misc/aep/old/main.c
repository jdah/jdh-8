#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>

#define F_CPU 16000000
#define BAUD 9600

#include <util/delay.h>
#include <util/setbaud.h>

struct Pin {
    volatile uint8_t *port, *ddr, *pin;
    uint8_t no;
};

const struct Pin IO_PINS[] = {
    { &PORTD, &DDRD, &PIND, 2 },
    { &PORTD, &DDRD, &PIND, 3 },
    { &PORTD, &DDRD, &PIND, 4 },
    { &PORTD, &DDRD, &PIND, 5 },
    { &PORTD, &DDRD, &PIND, 6 },
    { &PORTD, &DDRD, &PIND, 7 },
    { &PORTB, &DDRB, &PINB, 0 },
    { &PORTB, &DDRB, &PINB, 1 },
};

const struct Pin
    PIN_NWE     =  { &PORTB, &DDRB, &PINB, 2 },
    PIN_SER     =  { &PORTB, &DDRB, &PINB, 3 },
    PIN_RCLK    =  { &PORTB, &DDRB, &PINB, 4 },
    PIN_SRCLK   =  { &PORTB, &DDRB, &PINB, 5 },
    PIN_LED     =  { &PORTB, &DDRB, &PINB, 5 };

// clock cycle time
#define T_CLK 100

// write cycle time, max
#define T_WC 10000

enum RWMode {
  RW_READ,
  RW_WRITE
};

enum PinMode {
    PM_INPUT,
    PM_OUTPUT
};

static struct {
    char data[1024];
    size_t i, len;
} sbuf;

static size_t se_cnt = 0;

enum AEPCommand {
  // INPUT = { }, OUTPUT = { 0xFF }
  AEPC_READY = 1,

  // INPUT = { u16 addr, u16 len }, OUTPUT = { u8 data, 0xFF }
  AEPC_READ = 2,

  // INPUT = { u16 addr, u16 len, u8 data }, OUTPUT = { 0xFF }
  AEPC_WRITE = 3,
};

static uint8_t write_buffer[64];

// shamelessly stolen from the arduino stdlib
// github.com/arduino/ArduinoCore-samd/blob/master/cores/arduino/delay.c
static inline void delay(size_t us) {
    _delay_us((double) us);
}

static inline void pin_mode(struct Pin pin, enum PinMode mode) {
    *pin.ddr = (*pin.ddr & ~(1 << pin.no)) | ((mode == PM_OUTPUT ? 1 : 0) << pin.no);
}

static inline uint8_t pin_read(struct Pin pin) {
    return (*pin.pin & (1 << pin.no)) >> pin.no;
}

static inline void pin_write(struct Pin pin, uint8_t data) {
    *pin.port = (*pin.port & ~(1 << pin.no)) | (!!data << pin.no);
}

void shift_out(struct Pin pin_data, struct Pin pin_clock, uint8_t data) {
    uint8_t mask = 0x80;

    #pragma GCC unroll 8
    for (uint8_t i = 0; i < 8; i++) {
        pin_write(pin_data, data & mask);
        pin_write(pin_clock, 1);
        pin_write(pin_clock, 0);
        mask >>= 1;
    }
}

static void se_init() {
    // values configured with F_CPU and BAUD
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
    UCSR0A &= ~_BV(U2X0);
    UCSR0B |= _BV(RXCIE0); // enable interrupt
    UCSR0B |= _BV(RXEN0) | _BV(TXEN0); // rx + tx
    UCSR0C |= _BV(UCSZ01) | _BV(UCSZ00); // set frame: 8 data, 1 stop

    // enable global interrupts
    sei();
}

// UART ISR
ISR(USART_RX_vect) {
    loop_until_bit_is_set(UCSR0A, UDRE0);
    char data = UDR0;

    if (sbuf.len == sizeof(sbuf.data)) {
        sbuf.i = (sbuf.i + 1) % sizeof(sbuf.data);
        sbuf.len--;
    }

    sbuf.data[(sbuf.i + sbuf.len) % sizeof(sbuf.data)] = data;
    sbuf.len++;

    se_cnt++;
}

static void se_writec(char c) {
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
}

static int se_readc(char *out) {
    cli();

    if (sbuf.len == 0) {
        sei();
        return -1;
    }

    *out = sbuf.data[sbuf.i];
    sbuf.i = (sbuf.i + 1) % sizeof(sbuf.data);
    sbuf.len--;

    sei();
    return 0;
}

static void se_wait() {
    while (sbuf.len == 0) {
        delay(32768);
    }
}

static char se_wreadc() {
    se_wait();
    char c;
    assert(!se_readc(&c));
    return c;
}

static void se_writestr(const char *str) {
    while (*str) {
        se_writec(*str++);
    }
}

static void se_printf(const char *fmt, ...) {
    char buf[512];
    va_list va_args;
    va_start(va_args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va_args);
    se_writestr(buf);
    va_end(va_args);
}

void e_set_rw(enum RWMode mode) {
  for (int i = 0; i < 8; i++) {
    pin_mode(IO_PINS[i], mode == RW_READ ? PM_INPUT : PM_OUTPUT);
  }
}

void e_io_write(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    pin_write(IO_PINS[i], (data >> i) & 0x01);
  }
}

uint8_t e_io_read() {
  uint8_t res = 0;
  for (int i = 0; i < 8; i++) {
    res |= pin_read(IO_PINS[i]) << i;
  }
  return res;
}

void e_addr(uint16_t addr, bool oe) {
  shift_out(PIN_SER, PIN_SRCLK, (uint8_t) ((addr >> 8) | (((uint8_t) (!oe)) << 7)));
  shift_out(PIN_SER, PIN_SRCLK, (uint8_t) (addr & 0xFF));

  pin_write(PIN_RCLK, 0);
  pin_write(PIN_RCLK, 1);
  pin_write(PIN_RCLK, 0);
}

void e_write(uint16_t addr, uint8_t data) {
  e_set_rw(RW_WRITE);
  e_addr(addr, false);
  e_io_write(data);
  pin_write(PIN_NWE, 0);
  delay(1);
  pin_write(PIN_NWE, 1);
  delay(T_WC);

  // spin while write completes
  uint32_t n = 0;
  do {
    e_set_rw(RW_READ);
    e_addr(addr, true);
    n++;
  } while (e_io_read() != data);
}

bool e_can_write_page(uint16_t addr, uint16_t len) {
    return (addr & 0xFFA0) == ((addr + len) & 0xFFA0);
}

void e_write_page(uint16_t addr, uint8_t *data, uint16_t len) {
    e_set_rw(RW_WRITE);

    uint16_t i = 0;
    while (i != len) {
        e_addr(addr, false);
        e_io_write(data[i]);
        pin_write(PIN_NWE, 0);
        delay(1);
        pin_write(PIN_NWE, 1);
        delay(1);

        addr++;
        i++;
    }

    delay(T_WC);
}

void e_write_block(uint16_t addr, uint8_t *data, uint16_t len) {
    if (e_can_write_page(addr, len)) {
        e_write_page(addr, data, len);
    } else {
        uint16_t i = 0;
        while (i != len) {
            e_write(addr, data[i]);
            addr++;
            i++;
        }
    }
}

uint8_t e_read(uint16_t addr) {
  e_set_rw(RW_READ);
  e_addr(addr, true);
  pin_write(PIN_NWE, 1);
  return e_io_read();
}

void e_dump() {
  for (int i = 0; i < (1 << 12); i += 8) {
    uint8_t bytes[8];
    for (int j = 0; j < 8; j++) {
       bytes[j] = e_read(i + j);
    }

    se_printf(
      "%" PRIx8 " %" PRIx8 " %" PRIx8 " %" PRIx8
      " %" PRIx8 " %" PRIx8 " %" PRIx8 " %" PRIx8 "\r\n",
      bytes[0], bytes[1], bytes[2], bytes[3],
      bytes[4], bytes[5], bytes[6], bytes[7]
    );
  }
}

static void exec_command(enum AEPCommand cmd, uint16_t cmdlen) {
    uint16_t addr, len;

    switch (cmd) {
        case AEPC_READY:
            se_writec(0xFF);
            break;
        case AEPC_READ:
        case AEPC_WRITE:
            addr = ((uint16_t) se_wreadc()) | (((uint16_t) se_wreadc()) << 8);
            len = ((uint16_t) se_wreadc()) | (((uint16_t) se_wreadc()) << 8);

            if (cmd == AEPC_READ) {
                while (len != 0) {
                    se_writec(e_read(addr++));
                    len--;
                }
            } else {
                uint16_t chunk = 0, total = 0;

                while (total != len) {
                    while (chunk < sizeof(write_buffer) && (chunk + total != len)) {
                        write_buffer[chunk++] = se_wreadc();
                    }

                    e_write_block(addr + total, write_buffer, chunk);

                    total += chunk;
                    chunk = 0;
                }
            }

            se_writec(0xFF);
            break;
        default:
            se_writec(0xEE);
    }

    // check and reset serial read counter
    assert(se_cnt == cmdlen);
    se_cnt = 0;
}

void main() {
  se_init();

  pin_mode(PIN_NWE, PM_OUTPUT);
  pin_mode(PIN_SRCLK, PM_OUTPUT);
  pin_mode(PIN_RCLK, PM_OUTPUT);
  pin_mode(PIN_SER, PM_OUTPUT);
  pin_write(PIN_NWE, 1);

  // write startup code
  se_writec(0xAA);

  while (true) {
    enum AEPCommand cmd = se_wreadc();
    uint16_t len = ((uint16_t) se_wreadc()) | (((uint16_t) se_wreadc()) << 8);
    exec_command(cmd, len);
  }
}
