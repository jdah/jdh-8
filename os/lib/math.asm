; OS-BUNDLED MATH FUNCTIONS
; INCLUDED IN OSTEXT.ASM

; multiply
; a: first operand
; b: second operand
mul:
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

; multiply (16-bit x 16-bit)
; a, b: first operand
; c, d: second operand
; D0: temporary variable storage
mul16:
  push c, d, z
  eq16 c, d, 0
  jeq [.done]
  sw16 [D0], a, b
  mw16 a, b, 0
.loop:
  lw16 z, f, [D0]
  add16 a, b, z, f
  dec16 c, d
  eq16 c, d, 0
  jne [.loop]
.done:
  pop z, d, c
  ret

; compares two 16-bit numbers
; NOTE: DOES NOT SET CARRY/BORROW BITS
; a, b: first operand
; c, d: second operand
cmp16:
  push a, b
  cmp a, c
  mw h, f
  cmp b, d
  mw l, f

  ; less: high less or (high equal and low less)
  mw a, h
  mw b, a
  push h, l
  call [shr]
  pop l, h
  and a, l
  or a, h
  and a, 0x01

  ; equal: both equal
  mw b, h
  and b, l
  and b, 0x02
  or a, b
  mw f, a

  pop b, a
  ret
