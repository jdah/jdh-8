#define PIN_IO0   2
#define PIN_IO1   3
#define PIN_IO2   4
#define PIN_IO3   5
#define PIN_IO4   6
#define PIN_IO5   7
#define PIN_IO6   8
#define PIN_IO7   9

#define PIN_NWE   10

#define PIN_SER   11
#define PIN_RCLK  12
#define PIN_SRCLK 13

#define T_CLK 100
#define T_WC 10000

const int IO_PINS[8] = {
  PIN_IO0, PIN_IO1, PIN_IO2, PIN_IO3,
  PIN_IO4, PIN_IO5, PIN_IO6, PIN_IO7,
};

enum RW {
  RW_READ,
  RW_WRITE
};

uint8_t write_buffer[128];

void e_set_rw(RW mode) {
  for (int i = 0; i < 8; i++) {
    pinMode(IO_PINS[i], mode == RW_READ ? INPUT : OUTPUT);
  }
}

void e_io_write(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(IO_PINS[i], (data >> i) & 0x01);  
  }
}

uint8_t e_io_read() {
  uint8_t res = 0;
  for (int i = 0; i < 8; i++) {
    res |= digitalRead(IO_PINS[i]) << i;
  }
  return res;
}

void e_addr(uint16_t addr, bool oe) {
  shiftOut(PIN_SER, PIN_SRCLK, MSBFIRST, (uint8_t) ((addr >> 8) | (((uint8_t) (!oe)) << 7)));
  shiftOut(PIN_SER, PIN_SRCLK, MSBFIRST, (uint8_t) (addr & 0xFF));

  digitalWrite(PIN_RCLK, LOW);
  digitalWrite(PIN_RCLK, HIGH);
  digitalWrite(PIN_RCLK, LOW);
}

void e_write(uint16_t addr, uint8_t data) {
  e_set_rw(RW_WRITE);
  e_addr(addr, false);
  e_io_write(data);
  digitalWrite(PIN_NWE, LOW);
  delayMicroseconds(T_WC);
  digitalWrite(PIN_NWE, HIGH);
  delayMicroseconds(T_WC);
}

bool e_can_page_write(uint16_t addr, uint16_t len) {
  return (addr >> 6) == ((addr + len) >> 6);
}

void e_page_write(uint16_t addr, uint8_t *data, uint16_t len) {
  e_set_rw(RW_WRITE);

  for (int i = 0; i < len; i++) {
    e_addr(addr + i, false);
    e_io_write(data[i]);
    digitalWrite(PIN_NWE, LOW);
    delayMicroseconds(10);
    digitalWrite(PIN_NWE, HIGH);
  }

  delayMicroseconds(T_WC);
}

void e_write_multi(uint16_t addr, uint8_t *data, uint16_t len) {
//  if (e_can_page_write(addr, len)) {
//    e_page_write(addr, data, len);  
//  } else {
    for (uint16_t i = 0; i < len; i++) {
      e_write(addr + i, data[i]);
    }
//  }
}

uint8_t e_read(uint16_t addr) {
  digitalWrite(PIN_NWE, HIGH);
  e_set_rw(RW_READ);
  e_addr(addr, true);
  delayMicroseconds(T_CLK);
  return e_io_read();
}

void e_dump() {
  for (int i = 0; i < (1 << 12); i += 8) {
    uint8_t bytes[8];
    for (int j = 0; j < 8; j++) {
       bytes[j] = e_read(i + j);
    }

    char buffer[256];
    snprintf(
      buffer, 256,
      "%" PRIx8 " %" PRIx8 " %" PRIx8 " %" PRIx8
      " %" PRIx8 " %" PRIx8 " %" PRIx8 " %" PRIx8 "\r\n",
      bytes[0], bytes[1], bytes[2], bytes[3],
      bytes[4], bytes[5], bytes[6], bytes[7]
    );
    Serial.print(buffer);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_NWE, OUTPUT);
  pinMode(PIN_SRCLK, OUTPUT);
  pinMode(PIN_RCLK, OUTPUT);
  pinMode(PIN_SER, OUTPUT);
  digitalWrite(PIN_NWE, HIGH);

  while (Serial.available()) {
    read_wait();
  }

  // write startup code (0xAA)
  Serial.write(0xAA);
}

#define EOC()                   \
  Serial.write((uint8_t) 0xFF); \
  Serial.flush();               \

uint8_t read_wait() {
    int res;
    while ((res = Serial.read()) == -1);
    return (uint8_t) res;
}

void loop() {
  while (Serial.available() < 3) {
  }

  uint8_t cmd = read_wait();
  uint16_t cmdlen = ((uint16_t) read_wait()) | (((uint16_t) read_wait()) << 8);

  // wait for at least cmdlen - 3 more bytes
//  while (Serial.available() < (cmdlen - 3)) {
//    delay(10);
//  }

  if (cmd == 1 /* READY */) {
    EOC();
  } else if (cmd == 2 /* READ */) {
    uint16_t
      addr = ((uint16_t) read_wait()) | (((uint16_t) read_wait()) << 8),
      len = ((uint16_t) read_wait()) | (((uint16_t) read_wait()) << 8);

    while (len != 0) {
      Serial.write((uint8_t) e_read(addr++));
      len--;
    }

    EOC();
  } else if (cmd == 3 /* WRITE */) {
    uint16_t
      addr = ((uint16_t) read_wait()) | (((uint16_t) read_wait()) << 8),
      len  = ((uint16_t) read_wait()) | (((uint16_t) read_wait()) << 8);

    uint16_t i = 0;

    while (i != len) {
      write_buffer[i++] = read_wait();
    }

    e_write_multi(addr, (uint8_t *) write_buffer, len);

    EOC();
  } else {
    EOC();
  }
}
