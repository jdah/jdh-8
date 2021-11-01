import sys
import serial
import readline

import PIL
from PIL import Image

AEPC_READY = 1
AEPC_READ = 2
AEPC_WRITE = 3

READY = 0xAA
EOC = 0xFF

if len(sys.argv) != 3:
    print('Not enough arguments.')
    print('usage: aep.py <baud rate> <port>')
    exit(1)

se = serial.Serial(
    port=sys.argv[2],
    baudrate=int(sys.argv[1]),
)

def se_read():
    try:
        return se.read(1)[0]
    except KeyboardInterrupt:
        exit(1)
    except:
        print('Timeout :(')
        return 0

def se_read16():
    return se_read() | (se_read() << 8)

def se_write(data):
    if se.write(bytes([data & 0xFF])) != 1:
        raise Exception()

def se_write16(data):
    se_write((data >> 0) & 0xFF)
    se_write((data >> 8) & 0xFF)

def rom_read(addr, length):
    data = []
    se_write(AEPC_READ)
    se_write16(3 + 4)
    se_write16(addr)
    se_write16(length)

    for _ in range(0, length):
        data.append(se_read())

    eoc = se_read()
    if eoc != EOC:
        raise Exception('Invalid EOC 0x{:02X}'.format(eoc))

    return data

def rom_write(addr, data):
    se_write(AEPC_WRITE)
    se_write16(3 + 4 + len(data))
    se_write16(addr)
    se_write16(len(data))

    for x in data:
        se_write(x)

    eoc = se_read()
    if eoc != EOC:
        raise Exception('Invalid EOC 0x{:02X}'.format(eoc))

def rom_write_chunked(addr, data, chunk_size=48):
    chunks = [data[x:x+chunk_size] for x in range(0, len(data), chunk_size)]

    for i, chunk in enumerate(chunks):
        base = addr + (i * chunk_size)
        last = base + len(chunk) - 1

        rom_write(base, chunk)
        yield (base, last)

def rom_verify(addr, data, chunk_size=48):
    chunks = [data[x:x+chunk_size] for x in range(0, len(data), chunk_size)]

    for i, chunk in enumerate(chunks):
        base = addr + (i * chunk_size)
        last = base + len(chunk) - 1

        rom_chunk = rom_read(base, len(chunk))
        for j, (a, b) in enumerate(zip(chunk, rom_chunk)):
            if a != b:
                raise Exception(
                    'Verification failure at 0x{:04X}: expected 0x{:02X}, got 0x{:02X}'
                        .format(base + j, a, b))

        yield (base, last)

def rom_clear():
    CLEAR_BLOCK = 48

    print('CLEARING...')
    for i in range(0, 0xFFFF, CLEAR_BLOCK):
        rom_write(i, [0xFF] * CLEAR_BLOCK)
        print('  0x{:04X}'.format(i))

    print('CHECKING...')
    for i in range(0, 0xFFFF, CLEAR_BLOCK):
        data = rom_read(i, CLEAR_BLOCK)
        if any([x != 0xFF for x in data]):
            print('Error in 0x{:04X} block'.format(i))
        print('  0x{:04X}'.format(i))

    print('DONE')

# wait for startup
while se_read() != READY:
    pass

while True:
    line = input('> ')
    tokens = line.split(' ')
    cmd = tokens[0].lower()

    if cmd == 'help':
        print(r'''
> help                          : print this message
> ping                          : checks that programmer is ready
> read <addr>                   : read a single byte
> read <addr> <len>             : read n bytes
> write <addr> <data>           : write a single byte
> write <addr> <file> <verify?> : writes a file at the specified address
> image <addr> <path>           : writes an image at the specified address
> clear                         : zeroes out entire ROM
> quit                          : quit
        ''')
    elif cmd == 'quit':
        exit(0)
    elif cmd == 'clear':
        rom_clear()
    elif cmd == 'read':
        if len(tokens) == 1:
            print('Not enough arguments')
            continue

        try:
            addr = int(tokens[1], 0)
        except:
            print('Invalid address')
            continue

        n = 1

        if len(tokens) == 3:
            try:
                n = int(tokens[2], 0)
            except:
                print('Invalid length')
                continue

        # send command
        data = rom_read(addr, n)
        chunks = [data[x:x+8] for x in range(0, len(data), 8)]

        for chunk in chunks:
            for i, x in enumerate(chunk):
                sys.stdout.write('0x{:02X}{}'.format(x, ' ' if i != len(chunk) - 1 else ''))
            sys.stdout.write('\n')
    elif cmd == 'write':
        if len(tokens) == 1:
            print('Not enough arguments')
            continue

        try:
            addr = int(tokens[1], 0)
        except:
            print('Invalid address')
            continue

        byte, path = None, tokens[2]

        try:
            byte = int(tokens[2], 0)
        except:
            pass

        data = None

        if byte:
            data = [byte]
        else:
            try:
                with open(path, 'rb') as f:
                    data = f.read()
            except Exception as e:
                print('Error opening file: {}'.format(e))
                continue

        if addr + len(data) >= 2 ** 16:
            print('Too large to write')
            continue

        if len(data) > 48:
            for base, last in rom_write_chunked(addr, data):
                sys.stdout.write('[0x{:04X}..0x{:04X}] ... OK\n'.format(base, last))
        else:
            rom_write(addr, data)

        # verify
        if len(tokens) == 4 and tokens[3].lower() in ['true', '1', 't', 'y']:
            try:
                for base, last in rom_verify(addr, data):
                    sys.stdout.write('[0x{:04X}..0x{:04X}] ... VERIFIED\n'.format(base, last))
            except Exception as e:
                print(e)
    elif cmd == 'image':
        if len(tokens) != 3:
            print('\'image\' only accepts three arguments')
            continue

        try:
            addr = int(tokens[1], 0)
        except:
            print('Invalid address')
            continue

        im = Image.open(tokens[2]).convert('RGB')

        if im.size != (208, 240):
            print('Invalid image size, can only accept 208x240')
            continue

        pixels = list(im.getdata())

        # assemble data: each line starts at 6th byte out of 32
        data = []

        for j in range(0, 240):
            for i in range(0, 32):
                if i < 4 or i > 29:
                    data.append(0x55)
                    continue

                base = (j * 208) + ((i - 4) * 8)
                px = map(lambda p: p[0] != 0 or p[1] != 0 or p[2] != 0, pixels[base:base+8])
                b = 0

                for k, x in enumerate(px):
                    b |= x << k

                data.append(b)

        for base, last in rom_write_chunked(addr, data):
            sys.stdout.write('[0x{:04X}..0x{:04X}] ... OK\n'.format(base, last))

        try:
            for base, last in rom_verify(addr, data):
                sys.stdout.write('[0x{:04X}..0x{:04X}] ... VERIFIED\n'.format(base, last))
        except Exception as e:
            print(e)
    elif cmd == 'ping':
        se_write(AEPC_READY)
        se_write16(3)
        res = se_read()
        if res != EOC:
            print('Error, expected 0x{:02X} but got 0x{:02X}'.format(EOC, res))
            exit(1)
        else:
            print('pong!')
    else:
        print('Unrecognized command {}, type \'help\' for help'.format(cmd))
