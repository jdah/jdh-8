; MATH

; multiply
; a: first operand
; b: second operand
mul:
  clb
  push b, c
  mw c, b
  mw b, a
  mw a, 0
  jz c, [.done]
.loop:
  add a, b
  dec c
  jnz c, [.loop]
.done:
  pop c, b
  ret

; multiply (16-bit x 8-bit)
; a, b: first operand
; c: second operand
mul16_8:
  clb
  push c, d, z
  mw d, a
  mw z, b
  mw a, 0
  mw b, 0
  jz c, [.done]
.loop:
  add16 a, b, d, z
  dec c
  jnz c, [.loop]
.done:
  pop z, d, c
  ret
