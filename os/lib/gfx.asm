; OS-BUNDLED GRAPHICS FUNCTIONS
; INCLUDED IN OSTEXT.ASM

; sets pixel
; a: x
; b: y
; c: value
set_pixel:
  pusha

  ; calculate pixel address at VRAM
  ; 0x8004 + y * 32 + (x >> 3)
  mw z, c
  push a
  mw d, b
  mw c, 0
  add a, a
  adc d, d
  adc c, c
  add a, a
  adc d, d
  adc c, c
  add a, a
  adc d, d
  adc c, c
  add a, a
  adc d, d
  adc c, c
  add a, a
  adc d, d
  adc c, c
  add d, 4
  adc c, 0x80

  ; calculate byte mask (pixel bit is set, other bits clear)
  pop b
  and b, 7
  mw a, 1
  call [shl]

  ; load VRAM byte
  lw b, c, d

  ; set or clear pixel depending on value
  jnz z, [.fill]

  ; clear pixel using inverted mask
  xor a, 0xFF
  and b, a
  jmp [.write]
.fill:
  ; set pixel using mask
  or b, a

.write:
  ; write VRAM byte back
  sw c, d, b

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
