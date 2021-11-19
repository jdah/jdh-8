; OS-BUNDLED SHIFT (LEFT/RIGHT) OPS
; INCLUDED IN OSTEXT.ASM

; SHIFT LEFT
; a: number to shift
; b: number of times to shift
shl:
  push b
  jz b, [.done]
  clb
.shift:
  ; a + a equivalent to left shift
  add a, a
  dec b
  jnz b, [.shift]
.done:
  pop b
  ret

; SHIFT LEFT (16-bit)
; a, b: number to shift
; c: times to shift
shl16:
  push c
  jz c, [.done]
  clb
.shift:
  ; ab + ab equivalent to left shift
  add16 a, b, a, b
  dec c
  jnz c, [.shift]
.done:
  pop c
  ret

; SHIFT RIGHT
; a: number to shift
; b: number of times to shift
shr:
  push b, c
  jz b, [.done]
  mw c, a
  mw a, 0
.shift:
  jmn c, 0x80, [.try_64]
  add a, 64
  sub c, 128
.try_64:
  jmn c, 0x40, [.try_32]
  add a, 32
  sub c, 64
.try_32:
  jmn c, 0x20, [.try_16]
  add a, 16
  sub c, 32
.try_16:
  jmn c, 0x10, [.try_08]
  add a, 8
  sub c, 16
.try_08:
  jmn c, 0x08, [.try_04]
  add a, 4
  sub c, 8
.try_04:
  jmn c, 0x04, [.try_02]
  add a, 2
  sub c, 4
.try_02:
  jmn c, 0x02, [.next]
  add a, 1
  sub c, 2
.next:
  dec b
  jnz b, [.shift]
.done:
  pop c, b
  ret
