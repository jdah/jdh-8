@include "os/arch.asm"
@include "os/oscall.asm"

@org ADDR_RAM

jmp [main]

@include "programs/holiday/image.asm"

counter:
  @db 0

main:
  sw [ADDR_MB], 1
  lda a, b, [ADDR_BANK]
  lda c, d, [image]
  lda [(SCANLINE_WIDTH_BYTES * SCREEN_HEIGHT)]
  sw16 [D0], h, l
  call [memcpy16]
.loop:
  jmp [.loop]
