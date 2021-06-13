; TEST
; a = 11
; z = 1
; c = 0
; d = 0xFF

; set up stack so calls work as expected
lda [0xFEFF]
sw [0xFFFC], l
sw [0xFFFD], h

call [start]
halt

return_from_me:
  mw z, c
  ret

start:
  mw c, 0x10
  mw z, 0xFF
  call [return_from_me]

  mw c, 0xFF
  lda [return_from_me]
  mw a, h
  mw b, l
  call a, b
  mw d, z

  mw a, 1
  mw c, 10
  mw z, 10

  call [add_one]
  ret

add_one:
  add a, 1
  call [return_from_me]
  sbb c, 1
  jz c, [.done]
  call [add_one]
.done:
  ret
