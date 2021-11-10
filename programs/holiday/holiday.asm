@include "os/arch.asm"
@include "os/oscall.asm"

@org ADDR_RAM

jmp [main]

@include "programs/holiday/image.asm"

main:
  lda a, b, [ADDR_BANK]
  lda c, d, [image]
  lda f, z, (SCANLINE_WIDTH_BYTES * SCREEN_HEIGHT)

  call [memcpy]
  halt
