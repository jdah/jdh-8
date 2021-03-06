# converts an image file into JDH-8 assembly syntax'd @db statements

import sys
from PIL import Image

def convert(path, label):
    im = Image.open(path).convert('RGB')

    if im.size != (208, 240):
        raise Exception('Invalid image size, can only accept 208x240')

    pixels = list(im.getdata())
    data = []

    for j in range(0, 240):
        for i in range(0, 32):
            if i < 4 or i > 29:
                data.append(0)
            else:
                base = (j * 208) + ((i - 4) * 8)
                px = map(lambda p: p[0] != 0 or p[1] != 0 or p[2] != 0,
                         pixels[base:base+8])
                b = 0

                for k, x in enumerate(px):
                    b |= x << k

                data.append(b)

    lines = [data[x:x+16] for x in range(0, len(data), 16)]
    lines = map(lambda l: map(lambda x: '0x%02x' % x, l), lines)
    lines = map(lambda l: '@db '  + ', '.join(l), lines)
    lines = '\n'.join(lines)

    return f'''; GENERATED BY JDH-8 IMAGE CONVERTER imc.py
; {path}
{label}:
{lines}

; END OF IMAGE
'''

if __name__ == '__main__':
    path, label = '', 'image'

    if len(sys.argv) == 2:
        path = sys.argv[1]
        print('No label provided, using \'image\'', file=sys.stderr)
    elif len(sys.argv) == 3:
        path = sys.argv[2]
        label = sys.argv[1]
    else:
        print('usage: inc.py [label] [image file]')
        exit(1)

    try:
        print(convert(path, label), end='', file=sys.stdout)
    except Exception as e:
        print(f'Error: {str(e)}', file=sys.stderr)
