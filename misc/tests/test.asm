; JDH-8 BASIC HARDWARE TEST
; NO OS DEPENDENCIES

@include "os/arch.asm"

@org 0x0000

main:
  sw [ADDR_MB], 1
  lda a, b, [(ADDR_BANK + 8)]
  mw c, 0
.loop:
  ; [hl] = [data + c]
  lda [data]
  add16 h, l, c

  ; d <- data
  lw d

  ; [hl] = [ab] <- d
  lda a, b
  sw d

  ; next scanline
  add16 a, b, SCANLINE_WIDTH_BYTES
  inc c
  jne c, 24, [.loop]
.done:
  jmp [.done]

data:
  @db 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00
  @db 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00
  @db 0x00, 0x66, 0x66, 0x00, 0x81, 0xE7, 0x3C, 0x00

