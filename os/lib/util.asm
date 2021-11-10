; OS-BUNDLED UTILITY FUNCTIONS
; INCLUDED IN OSTEXT.ASM

; computes the length of a string
; a, b: string
; z: result
strlen:
  push a, b, c
  mw z, 0
.loop:
  lw c, a, b
  jeq c, 0, [.done]
  add16 a, b, 1
  add z, 1
  jmp [.loop]
.done:
  pop c, b, a
  ret

; copies memory
; a, b: dst
; c, d: src
; z: len
memcpy:
  pusha
  jz z, [.done]
.loop:
  lw f, a, b
  sw c, d, f
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
; [SP - 2][SP - 1]: len
memcpy16:
  pusha

  popa
  ret

; sets memory
; a, b: dst
; c, d: len
; z: value
memset:
  pusha
  add16 c, d, a, b
.loop:
  eq16 a, b, c, d
  jnz f, [.done]
  sw a, b, z
  add16 a, b, 1
  jmp [.loop]
.done:
  popa
  ret

