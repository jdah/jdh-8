import sys

INPUT_FILE = sys.argv[1]

data = []
correct = []

# short pulse: 28us @ 0.0v; 4us @ 0.3v
SHORT = []
SHORT.extend([(0, 0)] * (28 * 4))
SHORT.extend([(1, 0)] *  (4 * 4))

# broad pulse: 2us @ 0.0v; 30us @ 0.3v
BROAD = []
BROAD.extend([(0, 0)] *  (2 * 4))
BROAD.extend([(1, 0)] * (30 * 4))

# scanline: 4us @ 0.0v (hsync); 6us @ 0.3v (back porch);
#           40us data @ 0.3v or 1.0v; 2us @ 0.3v (front porch)
LINE_BLACK, LINE_WHITE = [], []

for l in [LINE_BLACK, LINE_WHITE]:
    l.extend([(0, 0)] * (4 * 4))
    l.extend([(1, 0)] * (6 * 4))
    l.extend([(1, 1 if l is LINE_WHITE else 0)] * (52 * 4))
    l.extend([(1, 0)] * (2 * 4))

CORRECT_DEF = [
    (5,     SHORT),
    (5,     BROAD),
    (37,    LINE_BLACK),
    (240,   LINE_WHITE),
    (27,    LINE_BLACK),
    (5,     BROAD)
]

for n, l in CORRECT_DEF:
    correct.extend(l * n)

print(len(correct))

correct = list(map(lambda e: (e[0],) + e[1], enumerate(correct)))

with open('correct.txt', 'wb+') as f:
    for (a, b, c) in correct:
        f.write('{} {} {}\n'.format(a, b, c).encode('utf-8'))

with open(INPUT_FILE) as f:
    lines = list(enumerate(f))

    while not lines[0][1].startswith('0 '):
        print('Skipping line \"{}\"'.format(lines[0][1].replace('\n', '')))
        lines = lines[1:]

    j = 0

    for i, l in lines:
        assert int(l.split(' ')[0]) == j, 'Error on line {}'.format(i)
        data.append(tuple(map(lambda s: int(s.strip()), l.split(' '))))
        j += 1

# compare
for i, (exp, act) in enumerate(zip(correct, data)):
    if exp != act:
        print('ERROR: {} did not match {}, got {}.\r'.format(i, exp, act))

        for j in range(max(i - 5, 0), min(i + 5, min(len(correct), len(data)))):
            print('  {} {}{}\r'.format(correct[j], data[j], ' < HERE' if j == i else ''))

        exit(1)

print('No errors!')
exit(0)
