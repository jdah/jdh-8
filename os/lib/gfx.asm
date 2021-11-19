; OS-BUNDLED GRAPHICS FUNCTIONS
; INCLUDED IN OSTEXT.ASM

; sets pixel
; a: x
; b: y
; c: value
set_pixel:
  pusha

  mw d, a
  mw z, c

  ; b already contains y
  ; [ab] -> scanline start
  mw a, 0
  mw c, SCANLINE_WIDTH_BYTES
  call [mul16_8]

  ; offset into VRAM
  add16 a, b, ADDR_BANK

  ; x >> 3 for byte
  push a, b
  mw a, d
  mw b, 3
  call [shr]
  mw c, a
  pop b, a

  ; add byte offset to scanline pointer
  add16 a, b, c

  ; add scanline offset
  add16 a, b, SCANLINE_OFFSET_BYTES

  ; 1 << (d & 0x7) is pixel mask
  push a, b
  and d, 0x7
  mw a, 1
  mw b, d
  call [shl]
  mw d, a
  pop b, a

  lw c, a, b
  jnz z, [.fill]
  not d
  and c, d
  jmp [.store]
.fill:
  or c, d
.store:
  sw a, b, c
  popa
  ret

; draws a font glyph
; a: glyph to draw
; c, d: x, y (x is in byte indices)
draw_char:
  pusha

  ; move x from screen space -> vram space
  add c, SCANLINE_OFFSET_BYTES

  ; [ab] -> first byte
  sub a, ' '
  mw b, a
  mw a, 0
  push c
  mw c, 8
  call [mul16_8]
  pop c
  add16 a, b, [font]

  ; [cd] -> first screen byte
  push a, b, c
  mw c, SCANLINE_WIDTH_BYTES
  mw a, 0
  mw b, d
  call [mul16_8]
  add16 a, b, ADDR_BANK
  pop c
  add16 a, b, c
  mw c, a
  mw d, b
  pop b, a

  mw z, 7
.loop:
  lw f, a, b
  sw c, d, f
  add16 a, b, 1
  add16 c, d, SCANLINE_WIDTH_BYTES
  dec z
  jnz z, [.loop]
.done:
  popa
  ret

; draws a string
; a, b: string
; c, d: x, y (x is in byte indices)
draw_str:
  pusha
.loop:
  lw z, a, b    ; load character
  jz z, [.done] ; stop if null terminator
  push a
  mw a, z
  call [draw_char]
  pop a
  inc16 a, b
  inc c
  jmp [.loop]
.done:
  popa
  ret
