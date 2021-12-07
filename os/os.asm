; JDH-8 OPERATING SYSTEM
; INCLUDED IN OSTEXT.ASM

@include "programs/holiday/image.asm"

osmain:
  sw [ADDR_MB], 1
  lda a, b, [ADDR_BANK]
  lda c, d, [image]
  lda [(SCANLINE_WIDTH_BYTES * SCREEN_HEIGHT)]
  sw16 [D0], h, l
  call [memcpy16]
.loop:
  jmp [.loop]

  sw16 [ADDR_SPL], 0xFEFF
  sw [ADDR_MB], 1

  ; clear screen
  lda a, b [ADDR_BANK]
  lda c, d, (SCANLINE_WIDTH_BYTES * SCREEN_HEIGHT)
  mw z, 0xFF
  call [memset]

  call [term_clear]

  mw z, 24
.loop0:
  lda a, b, [string]
  call [term_print_str]
  dec z
  jnz z, [.loop0]
.loop1:
  jmp [.loop1]

string:
  @ds "ABCDEFGHIJKLMNOPQRSTUVWXYZ\nZLKD\n"
