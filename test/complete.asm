; TEST
; z = 0x00
; a = 0xFF

@define changes 0x80C0
@define data 0x80C1

mw a, 0x01
sw [changes], a

mw a, 0x66
sw [data], a

jmp [start]

; fail condition is in z
fail:
	mw a, 0xEE
	halt

@macro
failne %i0, %r1, %i2:
	mw z, %i0
	jne %r1, %i2, [fail]

@macro
faileq %i0, %r1, %i2:
	mw z, %i0
	jeq %r1, %i2, [fail]

start:

; mw
@macro
mw_test %r0:
	mw a, 0x55
	mw %r0, 0xAB
	failne 0x01, %r0, 0xAB
	mw a, %r0
	failne 0x02, a, 0xAB
	mw a, 0x12
	mw %r0, a
	failne 0x03, %r0, 0x12

mw_test a
mw_test b
mw_test c
mw_test d

; test z manually
mw z, 0xAB
jne z, 0xAB, [fail]
mw a, z
jne a, 0xAB, [fail]
mw a, 0x12
mw z, a
jne z, 0x12, [fail]

; lw
lda [0xFEFE]
lda [data]
lw d
failne 0x04, d, 0x66

lw c, [data]
failne 0x05, c, 0x66

; sw
lda [0xFEFE]
lda [data]
mw a, 0x77
sw a
lw b
failne 0x06, b, 0x77
; jne b, 0x77, [fail]
lw c, [data]
failne 0x07, c, 0x77
; jne c, 0x77, [fail]

; push/pop
lda [0xFEFF]
sw [0xFFFC], l
sw [0xFFFD], h

; TODO: might be a bug with macro placement?
; this might codegen even though it should be skipped
@macro
stack_test:
	mw a, 0x01
	mw b, 0x02
	push a
	push b
	push a
	push b
	pop a
	pop b
	pop c
	pop d
	failne 0x08, a, 0x02 ; jne a, 0x02, [fail]
	failne 0x09, b, 0x01 ; jne b, 0x01, [fail]
	failne 0x0A, c, 0x02 ; jne c, 0x02, [fail]
	failne 0x0B, d, 0x01 ; jne d, 0x01, [fail]
	push 0x66
	pop a
	failne 0x0D, a, 0x66 ; jne a, 0x66, [fail]
	push a
	pop z
	jne z, 0x66, [fail]

stack_test

; lda
lda [0xEEEE]
failne 0x0E, h, 0xEE ; jne h, 0xEE, [fail]

lda [0xEEEE]
failne 0x0F, l, 0xEE ; jne l, 0xEE, [fail]

; jnz

; imm
lda [after]
jnz 1

skip_me:
	mw z, 0x10
	jz b, [fail]
	mw a, 0x02
	sw [changes], a
after:

; jumps second time
lda [changes]
lw c
jne c, 0x01, [move_on]

; reg
mw b, 0x00
lda [skip_me]
jnz b

mw b, 0x01
jnz b

move_on:

; arithmetic ops

; ADD
mw a, 0x01
mw b, 0x02
add a, b
failne 0x11, a, 0x03 ; jne a, 0x03, [fail]
failne 0x12, b, 0x02 ; jne b, 0x02, [fail]

; less and borrow
mw a, 0x01
mw b, 0x02
add a, b
failne 0x12, f, 0x09 ; jne f, 0x09, [fail]

; ADC
stc
mw a, 0x01
mw b, 0x02
adc a, b
mw c, f
failne 0x13, a, 0x04 ; jne a, 0x04, [fail]
failne 0x14, b, 0x02 ; jne b, 0x02, [fail]
failne 0x15, c, 0x09 ; jne c, 0x09, [fail]

; AND
mw f, 0x55
mw c, 0x75
mw d, 0x11
and c, d
failne 0x16, f, 0x55 ; jne f, 0x55, [fail]
failne 0x17, c, 0x11 ; jne c, 0x11, [fail]

; OR
mw f, 0x66
mw c, 0x75
mw d, 0x1F
or c, d
failne 0x18, f, 0x66 ; jne f, 0x66, [fail]
failne 0x19, c, 0x7f ; jne c, 0x7F, [fail]

; NOR
mw f, 0x77
mw c, 0x75
mw d, 0x1F
nor c, d
failne 0x1A, f, 0x77 ; jne f, 0x77, [fail]
failne 0x1B, c, 0x80 ; jne c, 0x80, [fail]

; CMP is tested implicitly by previous jumps

; SBB
clb
mw c, 0x0F
mw d, 0x04
sbb c, d
failne 0x1C, f, 0x00 ; jne f, 0x00, [fail]
failne 0x1D, c, 0x0B ; jne c, 0x0B, [fail]

stb
mw c, 0x0F
mw d, 0x04
sbb c, d
failne 0x1E, f, 0x00 ; jne f, 0x00, [fail]
failne 0x1F, c, 0x0A ; jne c, 0x0A, [fail]

; const arithmetic
clc
mw a, 0x01
add a, 0xFF
push f
failne 0x20, a, 0x00 ; jne a, 0x00, [fail]
pop f
failne 0x21, f, 0x0D ; jne f, 0x0D, [fail]

; no writes to ROM
mw a, 0xEE
sw [0x0000], a
lw b, [0x0000]
faileq 0x22, b, 0xEE ; jeq b, 0xEE, [fail]

; general purpose RAM
sw [0xF000], a
lw b, [0xF000]
failne 0x23, b, 0xEE ; jne b, 0xEE, [fail]

mw a, 0xDD
lda [0xF000]
sw a
lw b
failne 0x24, b, 0xDD ; jne b, 0xDD, [fail]

; banked memory (VRAM)
mw a, 0x66
sw [0x8080], a
lw b, [0x8080]
failne 0x25, b, 0x66 ; jne b, 0x66, [fail]

; switch bank
mw d, 0x01
sw [0xFFFA], d
lw b, [0x8080]
faileq 0x26, b, 0x66 ;  jeq b, 0x66, [fail]

mw a, 0x77
sw [0x8080], a
lw b, [0x8080]
failne 0x27, b, 0x77 ; jne b, 0x77, [fail]

; general purpose RAM, again, with banked RAM enabled
; 0xF000 should still have 0xDD in it!

lw b, [0xF000]
failne 0x28, b, 0xDD ; jne b, 0xDD, [fail]

; try overwriting again
mw a, 0x55
lda [0xF000]
sw a
lw b
failne 0x29, b, 0x55 ; jne b, 0x55, [fail]

; repeat stack test in banked RAM
lda [0x8FFF]
sw [0xFFFC], l
sw [0xFFFD], h

stack_test

; proc test
lda a, b, [string]
call [strlen]
mw a, z
failne 0x30, a, 4
jmp [after_strlen]

strlen:
  mw z, 0
.loop:
  lw d, a, b
  jeq d, 0, [.done]
  add16 a, b, 1
  add z, 1
  jmp [.loop]
.done:
  ret
after_strlen:

mw z, 0x00
mw a, 0xFF
halt

string:
	@ds "a bt"
