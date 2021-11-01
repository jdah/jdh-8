; TEST
; [0x8000] = 0

; set up stack
lda [0xFEFF]
sw [0xFFFC], l
sw [0xFFFD], h

mw a, 0x01
sw [0x8000], a
jmp [main]

fail:
  mw a, 0x01
  sw [0x8000], a
  mw d, 0xEE
  halt

main:
  lda a, b, [string]
  call [strlen]
  jne z, 13, [fail]

  lda a, b, [bytes]
  lw d, a, b
  jne d, 0xFF, [fail]
  add16 a, b, 1

  lw d, a, b
  jne d, 45, [fail]
  add16 a, b, 1

  lw d, a, b
  jne d, (-1), [fail]

  ; call set_z_to_one through address stored in data
  mw z, 0
  lda a, b, [double_words]
  add16 a, b, 4
  lw c, a, b
  add16 a, b, 1
  lw d, a, b
  call d, c
  jne z, 1, [fail]

  call [sets_a_to_ff]
  jne a, 0xFF, [fail]

  sw [0x8000], 0x00
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

set_z_to_one:
  mw z, 1
  ret

bytes:
  @db 0xFF, 45, (-1), 0

double_words:
  @dd 0xFF00, 0xFFFF, [set_z_to_one]

string:
  @ds "Hello, world!"

; should be able to just @db our own instructions if we want
sets_a_to_ff:
  @db 0x00 0xFF ; mw a, 0xFF
  ret
