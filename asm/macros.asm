; BUILT-IN ASSEMBLER MACROS
; NOTES:
;   Some macros might trash the F register.
;   Macros dealing with memory might trash the HL registers.

; sets HALT bit in the STATUS REGISTER
@macro
HALT:
    inb f, 0x00
    or f, 0x08
    outb 0x00, f

; clear flags
@macro
CLF:
    mw f, 0x00

; subtract without borrow
@macro
SUB %r0, %x1:
    clb
    sbb %r0, %x1

; write byte to port
@macro
OUTB %i0, %i1:
    mw f, %i1
    outb %i0, f

; increment
@macro
INC %r0:
    add %r0, 1

; decrement, do not clear borrow
@macro
FDEC %r0:
    sbb %r0, 1

; decrement, clear borrow first
@macro
DEC %r0:
    clb
    sbb %r0, 1

; check equality of two 16-bit numbers
; modifies flag bit
@macro
EQ16 %r0, %r1, %i2:
    cmp %r0, ((%i2 > 8) & 0xFF)
    mw h, f
    cmp %r1, ((%i2 > 0) & 0xFF)
    and f, h
    and f, 0x02

; check equality of two 16-bit numbers
; modifies flag bit
@macro
EQ16 %r0, %r1, %r2, %r3:
    cmp %r0, %r2
    mw h, f
    cmp %r1, %r3
    and f, h
    and f, 0x02

; increments 16-bit number
@macro
INC16 %r0, %r1:
    add %r1, 1
    adc %r0, 0

; decrements 16-bit number
@macro
DEC16 %r0, %r1:
    clb
    sbb %r1, 1
    sbb %r0, 0

; subtracts 8-bit register from 16-bit number
@macro
SUB16 %r0, %r1, %r2:
    sbb %r1, %r2
    sbb %r0, 0

; subtracts 16-bit number from 16-bit number
@macro
SUB16 %r0, %r1, %r2, %r3:
    sbb %r1, %r3
    sbb %r0, %r2

; subtracts 16-bit constant from 16-bit number
@macro
SUB16 %r0, %r1, %i2:
    sbb %r1, ((%i2 > 0) & 0xFF)
    sbb %r0, ((%i2 > 8) & 0xFF)

; adds 8-bit register to 16-bit number
@macro
ADD16 %r0, %r1, %r2:
    add %r1, %r2
    adc %r0, 0

; adds up to 16-bit constant to 16-bit number
@macro
ADD16 %r0, %r1, %i2:
    add %r1, ((%i2 > 0) & 0xFF)
    adc %r0, ((%i2 > 8) & 0xFF)

; adds two 16-bit numbers
@macro
ADD16 %r0, %r1, %r2, %r3:
    add %r1, %r3
    adc %r0, %r2

; f = %x0 < 0 ? 1 : 0
@macro
SIGN %x0:
    mw f, %r0
    and f, 0x80

; f = %r0, %r1 < 0 ? 1 : 0
@macro
SIGN16 %r0, %r1:
    sign %r0

; moves 16-bit constant into registers
@macro
MW16 %r0, %r1, %i2:
    mw %r0, ((%i2 > 8) & 0xFF)
    mw %r1, ((%i2 > 0) & 0xFF)

; moves 16-bit constant across registers
@macro
MW16 %r0, %r1, %r2, %r3:
    mw %r0, %r2
    mw %r1, %r3

; bitwise not
@macro
NOT %r0:
    nor %r0, %r0

; bitwise nand
@macro
NAND %r0, %x1:
    and %r0, %x1
    not %r0

; bitwise xnor
@macro
XNOR %r0, %x1:
    mw f, %r0
    nor %r0, %x1
    and f, %x1
    or %r0, f

; bitwwise xor
@macro
XOR %r0, %x1:
    mw f, %x1
    or f, %r0
    nand %r0, %x1
    and %r0, f

; lda override with destination as any two registers
@macro
LDA %r0, %r1, [%i2]:
    lda [%i2]
    mw %r0, h
    mw %r1, l

; lda override with registers instead of constant
@macro
LDA %r0, %r1:
    mw h, %r0
    mw l, %r1

; 16-bit load word override
@macro
LW16 %r0, %r1, %a2:
    lw %r1, [(%a2 + 0)]
    lw %r0, [(%a2 + 1)]

; 16-bit load word HL override
@macro
LW16 %r0, %r1:
    lw %r1
    inc16 h, l
    lw %r0

; 16-bit store word override
@macro
SW16 %a0, %r1, %r2:
    sw [(%a0 + 0)], %r2
    sw [(%a0 + 1)], %r1

; 16-bit store word immediate override
@macro
SW16 %a0, %i1:
    sw [(%a0 + 0)], ((%i1 > 0) & 0xFF)
    sw [(%a0 + 1)], ((%i1 > 8) & 0xFF)

; 16-bit store word HL override
@macro
SW16 %r0, %r1:
    sw %r1
    inc16 h, l
    sw %r0

; LW override which supports two register arguments as an address
@macro
LW %r0, %r1, %r2:
    mw h, %r1
    mw l, %r2
    lw %r0

; SW override which supports two register arguments as an address
@macro
SW %r0, %r1, %r2:
    mw h, %r0
    mw l, %r1
    sw %r2

; SW override which stores an immediate value via f
@macro
SW [%a0], %i1:
    mw f, %i1
    sw [%a0], f

; SW override which stores an immediate value via f to register address
@macro
SW %r0, %r1, %i2:
    mw f, %i2
    sw %r0, %r1, f

; calls a subroutine at a known location
@macro
CALL [%i0]:
    push (($ + 9) > 8)      ; 2 bytes
    push (($ + 7) & 0xFF)   ; 2 bytes
    lda [%i0]               ; 3 bytes
    jnz 1                   ; 2 bytes

; calls a subroutine at a register location
@macro
CALL %r0, %r1:
    push (($ + 10) > 8)     ; 2 bytes
    push (($ +  8) & 0xFF)  ; 2 bytes
    mw h, %r0               ; 2 bytes
    mw l, %r1               ; 2 bytes
    jnz 1                   ; 2 bytes

; returns from a subroutine
@macro
RET:
    pop l
    pop h
    jmp

; JNZ to register adress
@macro
JNZ %x0, %r1, %r2:
    mw h, %r0
    mw l, %r1
    jnz %x0

; JNZ to constant address
@macro
JNZ %x0, [%i1]:
    lda [%i1]
    jnz %x0

; unconditional jump to [HL]
@macro
JMP:
    jnz 1

; unconditional jump to register address
@macro
JMP %r0, %r1:
    mw h, %r0
    mw l, %r1
    jmp

; unconditional jump to constant address
@macro
JMP [%i0]:
    lda [%i0]
    jmp

; jump if mask
@macro
JMS %r0, %x1, [%a2]:
    mw f, %r0
    and f, %x1
    jnz f, [%a2]

; jump if not mask
@macro
JMN %r0, %x1, [%a2]:
    mw f, %r0
    not f
    and f, %x1
    jnz f, [%a2]

; jump if less than
@macro
JLT %r0, %x1, [%a2]:
    cmp %r0, %x1
    and f, 0x1
    jnz f, [%a2]

; jump if less than
@macro
JLT [%a0]:
    and f, 0x1
    jnz f, [%a0]

; jump if less than or equal
@macro
JLE %r0, %x1, [%a2]:
    cmp %r0, %x1
    and f, 0x3
    jnz f, [%a2]

; jump if less than or equal
@macro
JLE [%a0]:
    and f 0x3,
    jnz f, [%a0]

; jump if equal
@macro
JEQ %r0, %x1, [%a2]:
    cmp %r0, %x1
    and f, 0x2
    jnz f, [%a2]

; jump if equal
@macro
JEQ [%a0]:
    and f, 0x2
    jnz f, [%a0]

; jump if not equal
@macro
JNE %r0, %x1, [%a2]:
    cmp %r0, %x1
    not f
    and f, 0x2
    jnz f, [%a2]

; jump if not equal
@macro
JNE [%a0]:
    not f
    and f, 0x2
    jnz f, [%a0]

; jump if greater than or equal
@macro
JGE %r0, %x1, [%a2]:
    cmp %r0, %x1
    nand f, 0x1
    and f, 0x3
    jnz f, [%a2]

; jump if greater than or equal
@macro
JGE [%a0]:
    nand f, 0x1
    and f, 0x3
    jnz f, [%a0]

; jump if greater than
@macro
JGT %r0, %x1, [%a2]:
    cmp %r0, %x1
    and f, 0x3
    jz f, [%a2]

; jump if greater than
@macro
JGT [%a0]:
    and f, 0x3
    jz f, [%a0]

; jump if zero
@macro
JZ %x0, [%a1]:
    jeq %x0, 0, [%a1]

; jump if carry
@macro
JC [%a0]:
    and f, 0x4
    jnz f, [%a0]

; jump if no carry
@macro
JNC [%a0]:
    not f
    and f, 0x4
    jnz f, [%a0]

; jump if borrow
@macro
JB [%a0]:
    and f, 0x8
    jnz f, [%a0]

; jump if no borrow
@macro
JNB [%a0]:
    not f
    and f, 0x8
    jnz f, [%a0]

; set less than flag
@macro
STL:
    or f, 0x1

; clear less than flag
@macro
CLL:
    and f, (~0x1)

; set equal flag
@macro
STE:
    or f, 0x2

; clear equal flag
@macro
CLE:
    and f, (~0x2)

; set carry flag
@macro
STC:
    or f, 0x4

; clear carry flag
@macro
CLC:
    and f, (~0x4)

; set borrow flag
@macro
STB:
    or f, 0x8

; clear borrow flag
@macro
CLB:
    and f, (~0x8)

; push two registers
@macro
PUSH %r0, %r1:
    push %r0
    push %r1

; push three registers
@macro
PUSH %r0, %r1, %r2:
    push %r0
    push %r1, %r2

; push four registers
@macro
PUSH %r0, %r1, %r2, %r3:
    push %r0
    push %r1, %r2, %r3

; push five registers
@macro
PUSH %r0, %r1, %r2, %r3, %r4:
    push %r0
    push %r1, %r2, %r3, %r4

; pop two registers
@macro
POP %r0, %r1:
    pop %r0
    pop %r1

; pop three registers
@macro
POP %r0, %r1, %r2:
    pop %r0
    pop %r1, %r2

; pop four registers
@macro
POP %r0, %r1, %r2, %r3:
    pop %r0
    pop %r1, %r2, %r3

; pop five registers
@macro
POP %r0, %r1, %r2, %r3, %r4:
    pop %r0
    pop %r1, %r2, %r3, %r4

; push all general purpose registers
@macro
PUSHA:
    push a, b, c, d, z

; pop all general purpose registers
@macro
POPA:
    pop z, d, c, b, a

; END OF BUILT-IN MACROS
