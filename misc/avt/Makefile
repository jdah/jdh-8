CC=avr-gcc
LD=avr-gcc

CCFLAGS?=-Os -DF_APU=16000000UL -mmcu=atmega328p
LDFLAGS?=-mmcu=atmega328p

ARDUINO_USB = /dev/cu.usbmodem141201

SRCS=$(wildcard ./*.c)
OBJS=$(SRCS:.c=.o)

all: out

%.o: %.c
	$(CC) -o $@ -c $< $(CCFLAGS)

out: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

deploy: out
	avrdude -F -V -c arduino -p ATMEGA328p -P $(ARDUINO_USB) -b 115200 -U flash:w:out

clean:
	rm -f ./*.o out
