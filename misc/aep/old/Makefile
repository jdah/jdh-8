CC=avr-gcc
LD=avr-gcc

CCFLAGS?=-O1 -DF_APU=16000000UL -mmcu=atmega328p
LDFLAGS?=-mmcu=atmega328p

ARDUINO_USB = /dev/cu.usbmodem141301

all: out

%.o: %.c
	$(CC) -o $@ -c $< $(CCFLAGS)

out: main.o
	$(LD) $(CCFLAGS) $(LDFLAGS) -o $@ $^

deploy: out
	avrdude -F -V -c arduino -p ATMEGA328p -P $(ARDUINO_USB) -b 115200 -U flash:w:out

clean:
	rm -f ./*.o out aep
