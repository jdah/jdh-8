; TEST
; a = 0x03
; b = 0x04
; d = 0xFF
; z = 0x00

jmp [_c]

_a:
  mw a, 0x02
_b:
  mw a, 0x03
  jmp [_e]
_c:
  mw b, 0x04
  jmp [_a]
_d:
  mw c, 0x05
_e:
  mw d, 0xFF
  lda [_g]
  jnz d
_f:
  mw d, 0xEE
_g:
  mw z, 0x00
  lda [_f]
  jnz z
  halt
