; TEST
; a = 97
; b = 0x0a
; c = 98
; d = 0x09
; z = 0x5C

lda [0xFEFF]
sw [0xFFFC], l
sw [0xFFFD], h

jmp [test]

strings:
@ds "a\nb\t\\"

fail:
  mw a, 0xFF
  halt

test:
  lda a, b, [strings]
  call [strlen]
  jne z, 5, [fail]

  lw a, [(strings + 0)]
  lw b, [(strings + 1)]
  lw c, [(strings + 2)]
  lw d, [(strings + 3)]
  lw z, [(strings + 4)]
  halt

strlen:
  mw z, 0
.loop:
  lw d, a, b
  jeq d, 0, [.done]
  add16 a, b, 1
  add z, 1
  jmp [.loop]
.done:
  ret
