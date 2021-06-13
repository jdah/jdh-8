; ========
; = PONG =
; ========

; sizes
@define SCREEN_WIDTH  160
@define SCREEN_HEIGHT 120

@define PADDLE_HEIGHT 16
@define BALL_SIZE     1

; paddles
@define PLEFT 0
@define PRIGHT 1

; run out of characters after this...
@define MAX_SCORE 9

; keyboard port
@define KBPRT 3

@org 0x0000

jmp [main]

; DATA (ROM)
@include "lib/arch.asm"
@include "lib/shift.asm"
@include "lib/font.asm"
@include "lib/math.asm"

; DATA (RAM)
@define PADDLES (ADDR_RAM +  0)
@define PBALL   (ADDR_RAM +  2)
@define VBALL   (ADDR_RAM +  4)
@define SCORES  (ADDR_RAM +  6)
@define RANDN   (ADDR_RAM +  8)
@define DIRTY   (ADDR_RAM +  9)
@define PLBALL  (ADDR_RAM + 10)
@define KEYS    (ADDR_RAM + 12)

; dirty flags
@define DIRTY_SCORES  0x01
@define DIRTY_BORDER  0x02
@define DIRTY_PLEFT   0x04
@define DIRTY_PRIGHT  0x08

; key scancodes
@define SCAN_LU 0x1A
@define SCAN_LD 0x16
@define SCAN_RU 0x52
@define SCAN_RD 0x51

; key flags
@define KEY_LU  0x01
@define KEY_LD  0x02
@define KEY_RU  0x04
@define KEY_RD  0x08

; TEXT

; places random number in z
rand:
  push a
  lw z, [RANDN]
  mw a, 113
  add a, z
  sw [RANDN], a
  pop a
  ret

; resets the game
; a: last to score
reset:
  pusha

  ; any score > 9? reset scores
  lw a, [(SCORES + 0)]
  lw b, [(SCORES + 1)]
  cmp a, MAX_SCORE
  mw h, f
  cmp b, MAX_SCORE
  and f, h
  jle [.no_score_reset]
  sw [(SCORES + 0)], 0
  sw [(SCORES + 1)], 0
.no_score_reset:
  ; reset paddles
@define reset_PADDLE_HEIGHT ((SCREEN_HEIGHT - PADDLE_HEIGHT) / 2)
  sw [(PADDLES + 0)], reset_PADDLE_HEIGHT
  sw [(PADDLES + 1)], reset_PADDLE_HEIGHT

  ; reset ball
  sw [(PBALL + 0)], ((SCREEN_WIDTH - BALL_SIZE) / 2)

  call [rand]
  and z, 0x7F
  add z, ((SCREEN_HEIGHT - 0x7F) / 2)
  sw [(PBALL + 1)], z

  lw a, [(PBALL + 0)]
  lw b, [(PBALL + 1)]
  sw [(PLBALL + 0)], a
  sw [(PLBALL + 1)], b

  ; ball velocity -> towards last to lose
  jeq a, PRIGHT, [.neg_x]
  sw [(VBALL + 0)], 1
  jmp [.y]
.neg_x:
  sw [(VBALL + 0)], (-1)
.y:
  call [rand]
  jms z, 0x1, [.neg_y]
  sw [(VBALL + 1)], 1
  jmp [.done]
.neg_y:
  sw [(VBALL + 1)], (-1)
.done:
  ; reset screen
  lda a, b, [ADDR_BANK]
  lda c, d, [(SCREEN_HEIGHT * (SCREEN_WIDTH / 8))]
  mw z, 0
  call [memset]

  sw [DIRTY], 0xFF
  call [draw]
  sw [DIRTY], 0x00

  popa
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

; sets pixel
; a: x
; b: y
; c: value
set_pixel:
  pusha

  mw d, a
  mw z, c

  ; [ab] -> screen byte
  mw a, 0
  mw c, (SCREEN_WIDTH / 8)
  call [mul16_8]
  add16 a, b, ADDR_BANK

  ; x >> 3 for byte
  push a, b
  mw a, d
  mw b, 3
  call [shr]
  mw c, a
  pop b, a

  add16 a, b, c

  ; 1 << (d & 0x7) is pixel mask
  push a, b
  and d, 0x7
  mw a, 1
  mw b, d
  call [shl]
  mw d, a
  pop b, a

  lw c, a, b
  jnz z, [.fill]
  not d
  and c, d
  jmp [.store]
.fill:
  or c, d
.store:
  sw a, b, c
  popa
  ret

; draws a font glyph
; a: glyph to draw
; c, d: x, y (x is in byte indices)
draw_char:
  pusha
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
  mw c, (SCREEN_WIDTH / 8)
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
  add16 c, d, (SCREEN_WIDTH / 8)
  cdec z
  jnz z, [.loop]
.done:
  popa
  ret

; draws a paddle
; a: which paddle to draw
; b: which byte to draw
draw_paddle:
  pusha
  mw d, a
  mw z, b

  ; c <- paddle y
  lda [PADDLES]
  add16 h, l, a
  lw c

  ; [ab] -> first byte
  mw a, 0
  mw b, (SCREEN_WIDTH / 8)
  call [mul16_8]

  ; add paddle offset
  jeq d, PRIGHT, [.right]
  mw d, 1
  jmp [.cont]
.right:
  mw d, ((SCREEN_WIDTH / 8) - 2)
.cont:
  add16 a, b, d

  ; point into video RAM
  add16 a, b, ADDR_BANK

  ; [cd] -> last byte
  mw c, a
  mw d, b

  ; add (stride * PADDLE_HEIGHT)
  clb
  push z
  mw z, PADDLE_HEIGHT
.add:
  add16 c, d, (SCREEN_WIDTH / 8)
  dec z
  jnz z, [.add]
  pop z
.loop:
  sw a, b, z
  add16 a, b, (SCREEN_WIDTH / 8)
  eq16 a, b, c, d
  jz f, [.loop]
.done:
  popa
  ret

; draws the border
draw_border:
  pusha
  lda a, b, (ADDR_BANK + (((SCREEN_WIDTH / 8) / 2) - 1))
  mw c, 0
.loop:
  mw z, c
  and z, 0x01
  jnz z, [.next]
  lw d, a, b
  or d, 0x80
  sw a, b, d
  inc16 a, b
  lw d, a, b
  or d, 0x01
  sw a, b, d
  dec16 a, b
.next:
  add16 a, b, ((SCREEN_WIDTH / 8) * 3)
  add c, 3
  jlt c, SCREEN_HEIGHT, [.loop]
.done:
  popa
  ret

; draws scores
draw_scores:
  push a, c, d
  mw d, 4

  lw a, [(SCORES + 0)]
  add a, '0'
  mw c, (((SCREEN_WIDTH / 8) / 2) - 2)
  call [draw_char]

  lw a, [(SCORES + 1)]
  add a, '0'
  mw c, (((SCREEN_WIDTH / 8) / 2) + 1)
  call [draw_char]

  pop d, c, a
  ret

; clears scores
clear_scores:
  push a, c, d
  mw a, ' '
  mw d, 4

  mw c, (((SCREEN_WIDTH / 8) / 2) - 2)
  call [draw_char]

  mw c, (((SCREEN_WIDTH / 8) / 2) + 1)
  call [draw_char]

  pop d, c, a
  ret

; draws ball
; a: fill (>0) or clear (0)
draw_ball:
  pusha
  mw z, a
  mw c, 0
.loop_c:
  mw d, 0
.loop_d:
  lw a, [(PBALL + 0)]
  lw b, [(PBALL + 1)]

  add a, c
  add b, d
  push c
  mw c, z
  call [set_pixel]
  pop c
  inc d
  jne d, 2, [.loop_d]
  inc c
  jne c, 2, [.loop_c]

  popa
  ret

; dirty things according to ball's location
dirty_ball:
  push a, b, c
  lw a, [(PBALL + 0)]
  lw b, [(PBALL + 1)]
  lw c, [DIRTY]

  ; ball in middle? redraw border
  jlt a, ((SCREEN_WIDTH / 2) - 2), [.dscores]
  jgt a, ((SCREEN_WIDTH / 2) + 2), [.dscores]
  or c, DIRTY_BORDER
.dscores:
  jgt b, 16, [.dleft]
  or c, DIRTY_SCORES
.dleft:
  jgt a, 16, [.dright]
  or c, DIRTY_PLEFT
.dright:
  jlt a, (SCREEN_WIDTH - 16), [.done]
  or c, DIRTY_PRIGHT
.done:
  sw [DIRTY], c
  pop c, b, a
  ret

; renders everything which is dirty
draw:
  pusha

  ; clear ball
  lw a, [(PBALL + 0)]
  lw b, [(PBALL + 1)]
  lw c, [(PLBALL + 0)]
  lw d, [(PLBALL + 1)]
  sw [(PBALL + 0)], c
  sw [(PBALL + 1)], d

  push a, b
  mw a, 0
  call [draw_ball]
  pop b, a

  sw [(PBALL + 0)], a
  sw [(PBALL + 1)], b

  ; draw dirty screen elements
  lw c, [DIRTY]
  jmn c, DIRTY_BORDER, [.dscores]
  call [draw_border]
.dscores:
  jmn c, DIRTY_SCORES, [.dleft]
  call [draw_scores]
.dleft:
  jmn c, DIRTY_PLEFT, [.dright]
  mw a, PLEFT
  mw b, 0xF0
  call [draw_paddle]
.dright:
  jmn c, DIRTY_PRIGHT, [.done]
  mw a, PRIGHT
  mw b, 0x0F
  call [draw_paddle]
.done:
  sw [DIRTY], 0
  mw a, 1
  call [draw_ball]
  popa
  ret

; updates KEYS with current keyboard data
check_keys:
  push a, b, c

  lw c, [KEYS]
  inb a, KBPRT

  ; b <- pure scancode
  mw b, a
  and b, 0x7F

  ; determine key from scancode
.left_up:
  jne b, SCAN_LU, [.left_down]
  mw b, KEY_LU
  jmp [.cont]
.left_down:
  jne b, SCAN_LD, [.right_up]
  mw b, KEY_LD
  jmp [.cont]
.right_up:
  jne b, SCAN_RU, [.right_down]
  mw b, KEY_RU
  jmp [.cont]
.right_down:
  jne b, SCAN_RD, [.done]
  mw b, KEY_RD

.cont:
  jmn a, 0x80, [.down]
  ; clear bit (key up)
  not b
  and c, b
  jmp [.done]
.down:
  ; set bit (key down)
  or c, b
.done:
  sw [KEYS], c
  pop c, b, a
  ret

; move a paddle
; a: which paddle
; b: direction (-1 or 1)
move_paddle:
  pusha

  ; z <- ([cd] -> paddle byte)
  lda c, d, [PADDLES]
  add16 c, d, a
  lw z, c, d

  ; check that movement is OK
  jeq b, 1, [.down]
  jeq z, 0, [.done]
  jmp [.clear]
.down:
  jgt z, (SCREEN_HEIGHT - PADDLE_HEIGHT), [.done]

.clear:
  ; clear paddle
  push b
  mw b, 0x00
  call [draw_paddle]
  pop b

  ; add and store
  add z, b
  sw c, d, z

  ; mark dirty
  lw c, [DIRTY]
  jeq a, PRIGHT, [.right]
  or c, DIRTY_PLEFT
  jmp [.mark]
.right:
  or c, DIRTY_PRIGHT
.mark:
  sw [DIRTY], c
.done:
  popa
  ret

; move_ball z flags
@define MB_INV_X    0x01
@define MB_INV_Y    0x02
@define MB_RESET_L  0x04
@define MB_RESET_R  0x08

; moves the ball
; returns z, combination of above flags according to move
move_ball:
  push a, b, c, d

  mw z, 0

  ; try movement by velocity
  lw a, [(PBALL + 0)]
  lw b, [(PBALL + 1)]
  lw c, [(VBALL + 0)]
  lw d, [(VBALL + 1)]
  sw [(PLBALL + 0)], a
  sw [(PLBALL + 1)], b
  add a, c
  add b, d

.left_paddle:
  jgt a, (12 + BALL_SIZE), [.right_paddle]
  jlt a, (12 + BALL_SIZE - 4), [.right_paddle]
  lw c, [(PADDLES + 0)]
  mw d, c
  add d, PADDLE_HEIGHT
  jlt b, c, [.right_paddle]
  jgt b, d, [.right_paddle]
  mw z, MB_INV_X
  lw c, [KEYS]
  and c, (KEY_LU | KEY_LD)
  jz c, [.left_noinv]
  jeq c, KEY_LD, [.left_down]
  mw c, (-1)
  jmp [.left_check]
.left_down:
  mw c, 1
.left_check:
  lw d, [(VBALL + 1)]
  jeq c, d, [.left_noinv]
  or z, MB_INV_Y
.left_noinv:
  jmp [.apply]
.right_paddle:
  jlt a, (SCREEN_WIDTH - 16 - BALL_SIZE), [.top_bound]
  jgt a, (SCREEN_WIDTH - 16 - BALL_SIZE + 4), [.top_bound]
  lw c, [(PADDLES + 1)]
  mw d, c
  add d, PADDLE_HEIGHT
  jlt b, c, [.top_bound]
  jgt b, d, [.top_bound]
  mw z, MB_INV_X
  lw c, [KEYS]
  and c, (KEY_RU | KEY_RD)
  jz c, [.right_noinv]
  jeq c, KEY_RD, [.right_down]
  mw c, (-1)
  jmp [.right_check]
.right_down:
  mw c, 1
.right_check:
  lw d, [(VBALL + 1)]
  jeq c, d, [.right_noinv]
  or z, MB_INV_Y
.right_noinv:
  jmp [.apply]
.top_bound:
  jne b, 0, [.bottom_bound]
  mw z, MB_INV_Y
  jmp [.apply]
.bottom_bound:
  jne b, (SCREEN_HEIGHT - BALL_SIZE), [.left_bound]
  mw z, MB_INV_Y
  jmp [.apply]
.left_bound:
  jne a, 0, [.right_bound]
  lw a, [(SCORES + 1)]
  inc a
  sw [(SCORES + 1)], a
  mw z, MB_RESET_L
  jmp [.apply]
.right_bound:
  jne a, (SCREEN_WIDTH - BALL_SIZE), [.apply]
  lw a, [(SCORES + 0)]
  inc a
  sw [(SCORES + 0)], a
  mw z, MB_RESET_R
.apply:
  jz z, [.store]
  mw f, z
  and f, (MB_RESET_L | MB_RESET_R)
  jz f, [.invert]
  jeq z, MB_RESET_R, [.right_score]
  mw a, PLEFT
  jmp [.do_reset]
.right_score:
  mw a, PRIGHT
.do_reset:
  call [reset]
  jmp [.done]
.invert:
  ; invert according to z
  jmn z, MB_INV_X, [.invert_y]
  lw a, [(VBALL + 0)]
  mw b, 0
  sub b, a
  sw [(VBALL + 0)], b
.invert_y:
  jmn z, MB_INV_Y, [.done]
  lw a, [(VBALL + 1)]
  mw b, 0
  sub b, a
  sw [(VBALL + 1)], b
  jmp [.done]
.store:
  sw [(PBALL + 0)], a
  sw [(PBALL + 1)], b
.done:
  pop d, c, b, a
  ret

main:
  ; stack pointer
  lda [ADDR_STACK]
  sw [ADDR_SPL], l
  sw [ADDR_SPH], h

  ; memory bank -> screen
  lda 0x0001
  sw [ADDR_MBL], l
  sw [ADDR_MBH], h

  ; seed random number generator
  sw [RANDN], 13

  call [reset]

  mw z, 0
.loop:
  call [check_keys]

  ; busy-wait, expects 1 MHz clock
  mw c, 16
  call [mul16_8]
  inc z
  jne z, 144, [.loop]
  mw z, 0

  ; paddle movement
  lw c, [KEYS]
.move_lu:
  mw a, PLEFT
  jmn c, KEY_LU, [.move_ld]
  mw b, (-1)
  call [move_paddle]
.move_ld:
  jmn c, KEY_LD, [.move_ru]
  mw b, 1
  call [move_paddle]
.move_ru:
  mw a, PRIGHT
  jmn c, KEY_RU, [.move_rd]
  mw b, (-1)
  call [move_paddle]
.move_rd:
  jmn c, KEY_RD, [.move_done]
  mw b, 1
  call [move_paddle]
.move_done:

  ; update
  call [move_ball]

  ; loop again, do not render, if there was a reset
  jms z, (MB_RESET_L | MB_RESET_R), [.loop]

  ; render
  call [dirty_ball]
  call [draw]
  jmp [.loop]
  halt
