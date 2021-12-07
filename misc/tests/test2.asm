@org 0x0000

@include "os/arch.asm"

@define SEED   0x8000
@define OFFSET 0x8001

sw [ADDR_MB], 0x01
lda a, b, [0x0000]

loop:
  lda [image]
  add16 h, l, a, b
  lw c
  lda [ADDR_BANK]
  add16 h, l, a, b
  sw c
  add16 a, b, 0x01
  eq16 a, b, (SCANLINE_WIDTH_BYTES * SCREEN_HEIGHT)
  jne [loop]

; fib
fib:
    mw a, 0
    mw b, 1
    mw c, 9
.loop3:
    mw d, a
    add d, b
    mw a, b
    mw b, d
    clb
    sbb c, 1
    jnz c, [.loop3]

; result
sw [(ADDR_BANK + (SCANLINE_WIDTH_BYTES * 16) + 16)], d

spin:
  jmp [spin]

@include "programs/holiday/image.asm"
