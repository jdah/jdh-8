; TEST
; a = 0x00
; [0x8000] = 0xAA

jmp [main]

fail:
  mw a, 0x01
  halt

main:
  ; CHECK CMP F
  mw f, 0x08
  cmp f, 0x08
  and f, 0x02
  jz f, [fail]

  ; CHECK EQ16
  lda a, b, [fail]
  eq16 a, b, [fail]
  jz f, [fail]

  lda a, b, 0xFF04
  lda c, d, 0xFE03

  eq16 a, b, c, d
  jnz f, [fail]

  inc c
  inc d
  eq16 a, b, c, d
  jz f, [fail]

  ; CHECK INC16
  inc16 a, b
  eq16 a, b, [fail]
  jnz f, [fail]

  ; CHECK ADD16
  lda a, b, 0x00FF
  add16 a, b, 0x8001
  eq16 a, b,  0x8100
  jz f, [fail]

  ; CHECK SW AT REGISTER
  lda a, b, 0x8000
  sw a, b, 0xAA

  ; CHECK JMS
  mw a, 0xA2
  jms a, 0x02, [.continue_0]
  jmp [fail]
.continue_0:
  ; CHECK JMN
  mw a, 0xA2
  jmn a, 0x01, [.continue_1]
  jmp [fail]
.continue_1:
  ; CHECK JGT
  mw a, 0x3
  jgt a, 0x01, [.continue_2]
  jmp [fail]
.continue_2:
  mw a, 0x01
  jgt a, 0x02, [fail]

  mw a, 0x00
  halt
