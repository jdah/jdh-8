; BUILT-IN ASSEMBLER MACROS
; NOTES:
;   Some macros might trash the F register.
;   Macros dealing with memory might trash the HL registers.

; Sets HALT bit to one in STATUS REGISTER
@macro
HALT:
    inb f, 0x00
    or f, 0x08
    outb 0x00, f

@macro
CLF:
    mw f, 0x00

@macro
SUB %r0, %x1:
    clb
    sbb %r0, %x1

@macro
OUTB %i0, %i1:
    mw f, %i1
    outb %i0, f

@macro
INC %r0:
    add %r0, 1

@macro
DEC %r0:
    sbb %r0, 1

@macro
CDEC %r0:
    clb
    sbb %r0, 1

@macro
EQ16 %r0, %r1, %i2:
    cmp %r0, ((%i2 > 8) & 0xFF)
    mw h, f
    cmp %r1, ((%i2 > 0) & 0xFF)
    and f, h
    and f, 0x02

@macro
EQ16 %r0, %r1, %r2, %r3:
    cmp %r0, %r2
    mw h, f
    cmp %r1, %r3
    and f, h
    and f, 0x02

@macro
INC16 %r0, %r1:
    add %r1, 1
    adc %r0, 0

@macro
DEC16 %r0, %r1:
    clb
    sbb %r1, 1
    sbb %r0, 0

@macro
ADD16 %r0, %r1, %r2:
    add %r1, %r2
    adc %r0, 0

@macro
ADD16 %r0, %r1, %i2:
    add %r1, ((%i2 > 0) & 0xFF)
    adc %r0, ((%i2 > 8) & 0xFF)

@macro
ADD16 %r0, %r1, %r2, %r3:
    add %r1, %r3
    adc %r0, %r2

@macro
MW16 %r0, %r1, %i2:
    mw %r0, ((%i2 > 8) & 0xFF)
    mw %r1, ((%i2 > 0) & 0xFF)

@macro
MW16 %r0, %r1, %r2, %r3:
    mw %r0, %r2
    mw %r1, %r3

@macro
NOT %r0:
    nor %r0, %r0

@macro
NAND %r0, %x1:
    and %r0, %x1
    not %r0

@macro
XNOR %r0, %x1:
    mw f, %r0
    nor %r0, %x1
    and f, %x1
    or %r0, f

@macro
XOR %r0, %x1:
    mw f, %x1
    or f, %r0
    nand %r0, %x1
    and %r0, f

@macro
LDA %r0, %r1, [%i2]:
    lda [%i2]
    mw %r0, h
    mw %r1, l

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

; Calls a subroutine at a known location
@macro
CALL [%i0]:
    push (($ + 9) > 8)      ; 2 bytes
    push (($ + 7) & 0xFF)   ; 2 bytes
    lda [%i0]               ; 3 bytes
    jnz 1                   ; 2 bytes

; Calls a subroutine at a register location
@macro
CALL %r0, %r1:
    push (($ + 10) > 8)     ; 2 bytes
    push (($ +  8) & 0xFF)  ; 2 bytes
    mw h, %r0               ; 2 bytes
    mw l, %r1               ; 2 bytes
    jnz 1                   ; 2 bytes

; Returns from a subroutine
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

; Unconditional jump to [HL]
@macro
JMP:
    jnz 1

; Unconditional jump to register address
@macro
JMP %r0, %r1:
    mw h, %r0
    mw l, %r1
    jmp

; Unconditional jump to constant address
@macro
JMP [%i0]:
    lda [%i0]
    jmp

; Jump if mask
@macro
JMS %r0, %x1, [%a2]:
    mw f, %r0
    and f, %x1
    jnz f, [%a2]

; Jump if not mask
@macro
JMN %r0, %x1, [%a2]:
    mw f, %r0
    not f
    and f, %x1
    jnz f, [%a2]

; Jump if less than
@macro
JLT %r0, %x1, [%a2]:
    cmp %r0, %x1
    and f, 0x1
    jnz f, [%a2]

@macro
JLT [%a0]:
    and f, 0x1
    jnz f, [%a0]

; Jump if less than or equal
@macro
JLE %r0, %x1, [%a2]:
    cmp %r0, %x1
    and f, 0x3
    jnz f, [%a2]

@macro
JLE [%a0]:
    and f 0x3,
    jnz f, [%a0]

; Jump if equal
@macro
JEQ %r0, %x1, [%a2]:
    cmp %r0, %x1
    and f, 0x2
    jnz f, [%a2]

@macro
JEQ [%a0]:
    and f, 0x2
    jnz f, [%a0]

; Jump if not equal
@macro
JNE %r0, %x1, [%a2]:
    cmp %r0, %x1
    not f
    and f, 0x2
    jnz f, [%a2]

@macro
JNE [%a0]:
    not f
    and f, 0x2
    jnz f, [%a0]

; Jump if greater than or equal
@macro
JGE %r0, %x1, [%a2]:
    cmp %r0, %x1
    nand f, 0x1
    and f, 0x3
    jnz f, [%a2]

@macro
JGE [%a0]:
    nand f, 0x1
    and f, 0x3
    jnz f, [%a0]

; Jump if greater than
@macro
JGT %r0, %x1, [%a2]:
    cmp %r0, %x1
    and f, 0x3
    jz f, [%a2]

@macro
JGT [%a0]:
    and f, 0x3
    jz f, [%a0]

@macro
JZ %x0, [%a1]:
    jeq %x0, 0, [%a1]

@macro
JC [%a0]:
    and f, 0x4
    jnz f, [%a0]

@macro
JNC [%a0]:
    not f
    and f, 0x4
    jnz f, [%a0]

@macro
JB [%a0]:
    and f, 0x8
    jnz f, [%a0]

@macro
JNB [%a0]:
    not f
    and f, 0x8
    jnz f, [%a0]

@macro
STL:
    or f, 0x1

@macro
CLL:
    and f, (~0x1)

@macro
STE:
    or f, 0x2

@macro
CLE:
    and f, (~0x2)

@macro
STC:
    or f, 0x4

@macro
CLC:
    and f, (~0x4)

@macro
STB:
    or f, 0x8

@macro
CLB:
    and f, (~0x8)

@macro
PUSH %r0, %r1:
    push %r0
    push %r1

@macro
PUSH %r0, %r1, %r2:
    push %r0
    push %r1, %r2

@macro
PUSH %r0, %r1, %r2, %r3:
    push %r0
    push %r1, %r2, %r3

@macro
PUSH %r0, %r1, %r2, %r3, %r4:
    push %r0
    push %r1, %r2, %r3, %r4

@macro
POP %r0, %r1:
    pop %r0
    pop %r1

@macro
POP %r0, %r1, %r2:
    pop %r0
    pop %r1, %r2

@macro
POP %r0, %r1, %r2, %r3:
    pop %r0
    pop %r1, %r2, %r3

@macro
POP %r0, %r1, %r2, %r3, %r4:
    pop %r0
    pop %r1, %r2, %r3, %r4

@macro
PUSHA:
    push a, b, c, d, z

@macro
POPA:
    pop z, d, c, b, a

; END OF BUILT-IN MACROS
