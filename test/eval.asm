; TEST
; a = 0x00

mw a, (5 + 3)
jne a, 8, [fail]

mw a, (6 - 3)
jne a, 3, [fail]

mw a, ((5 + 6) + (3 - 2))
jne a, 12, [fail]

mw a, (1 < 3)
jne a, 0x8, [fail]

mw a, (1 < 3 + 5)
jne a, 13, [fail]

mw a, (~(1 < 3))
jne a, 0xF7, [fail]

mw a, (0x8 > 1)
jne a, 0x4, [fail]

mw a, (0x1 | (0xF0 & 0xF7))
jne a, 0xF1, [fail]

mw a, (0x1F ^ 0xF0)
jne a, 0xEF, [fail]

mw a, (10 / 5)
jne a, 2, [fail]

mw a, (100 / 30)
jne a, 3, [fail]

mw a, (-1)
jne a, 0xFF, [fail]

mw a, (~(((50 + (60 - 10)) / 100) < 3))
jne a, 0xF7, [fail]

mw a, 0x00
halt

fail:
  mw a, 0x01
  halt

@ds "aaaah"
