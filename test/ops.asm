; TEST
; a = 0x00

@define F_LESS 0x01
@define F_EQUAL 0x02
@define F_CARRY 0x03
@define F_BORROW 0x04

; cmp
mw a, 0x06
cmp a, 0x04
and f, (F_EQUAL | F_LESS)
jnz f, [fail]

; add
mw a, 0x05
mw b, 0x05
add a, b
cmp a, 0x0A
not f
and f, F_EQUAL
jnz f, [fail]

add a, a
cmp a, 0x14
not f
and f, F_EQUAL
jnz f, [fail]

; adc
mw a, 0xFF
mw b, 0x00
add a, b
jc [fail]

mw b, 0x01
add a, b
jnc [fail]

or f, 0x04
mw a, 0x00
mw b, 0x80
adc a, b
jne a, 0x81, [fail]

; and
mw a, 0xF0
mw b, 0x0F
and a, b
jnz a, [fail]

mw a, 0x80
mw b, 0x88
and a, b
jne a, 0x80, [fail]

; or
mw a, 0xF0
mw b, 0x0F
or a, b
jne a, 0xFF, [fail]

; nor
mw a, 0x80
mw b, 0x18
nor a, b
jne a, 0x67, [fail]

; sbb
mw a, 0x01
mw b, 0x01
sbb a, b
jb [fail]

sbb a, b
jnb [fail]

or f, 0x8
mw a, 0x05
mw b, 0x01
sbb a, b
jne a, 0x03, [fail]

; arithmetic macros
; not
mw a, 0x0F
not a
jne a, 0xF0, [fail]

; nand
mw a, 0x0F
mw b, 0xF0
nand a, b
jne a, 0xFF, [fail]

; xnor
mw a, 0xF0
mw b, 0x88
xnor a, b
jne a, 0x87, [fail]

; xor
mw a, 0xF0
mw b, 0x88
xor a, b
jne a, 0x78, [fail]

mw a, 0x00
halt

fail:
  mw a, 0x01
  halt
