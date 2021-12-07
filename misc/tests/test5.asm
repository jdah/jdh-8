@include "os/arch.asm"

@org 0x0000
@define Q0 0xA000

sw16 [ADDR_SPL], 0xFEFF
sw [ADDR_MB], 1
sw [(ADDR_BANK + 7)], 0xEE

lda a, b, [ADDR_BANK]
lda c, d, [image]
lda [(SCANLINE_WIDTH_BYTES * SCREEN_HEIGHT)]
sw16 [Q0], h, l
call [memcpy16]

loop2:
  jmp [loop2]

; copies memory
; a, b: dst
; c, d: src
; z: len
memcpy:
  pusha
  jz z, [.done]
.loop:
  lw f, c, d
  sw a, b, f
  add16 a, b, 1
  add16 c, d, 1
  dec z
  jnz z, [.loop]
.done:
  popa
  ret

; copies memory (16-bit size)
; a, b: dst
; c, d: src
; Q0: len
memcpy16:
  pusha
.loop:
  push a
  lw a, [(Q0 + 1)]
  jz a, [.less_than_256]
  push b
  lw b, [(Q0 + 0)]
  mw z, 0xFF
  clb
  sub16 a, b, 0xFF
  sw16 [Q0], a, b
  pop b, a
  call [memcpy]
  add16 a, b, 0xFF
  add16 c, d, 0xFF
  jmp [.loop]
.less_than_256:
  pop a
  lw z, [(Q0 + 0)]
  call [memcpy]
  popa
  ret

@include "programs/holiday/image.asm"
