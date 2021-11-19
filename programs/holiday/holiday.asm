@include "os/arch.asm"
@include "os/oscall.asm"

@org ADDR_RAM

sw [(ADDR_BANK + 4)], 0x00
halt

jmp [main]

@include "programs/holiday/image.asm"

counter:
  @db 0

main:
  lda a, b, [ADDR_BANK]
  lda c, d, [image]
  lda [(SCANLINE_WIDTH_BYTES * SCREEN_HEIGHT)]
  sw16 [D0], h, l
  call [memcpy16]
  sw [(ADDR_BANK + 4)], 0xAA

  mw a, 10
  mw b, 10
  mw c, 1
  call [set_pixel]

  halt
