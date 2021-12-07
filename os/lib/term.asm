; OS-BUNDLED TERMINAL FUNCTIONS
; INCLUDED IN OSTEXT.ASM

@define TERM_WIDTH  (192 / 8)
@define TERM_HEIGHT (240 / 8)
@define DATA        (OSDATA_OFFSET + 0)
@define DATA_SIZE   (TERM_WIDTH * TERM_HEIGHT)
@define CURSOR      (DATA + DATA_SIZE + 0)
@define CURSOR_X    (CURSOR + 0)
@define CURSOR_Y    (CURSOR + 1)

; gets a character
; a: x
; b: y
; z: result
term_lookup:
  push c, d

  ; c, d <- offset
  push a, b
  mw a, 0
  mw c, TERM_WIDTH
  call [mul16_8]
  mw16 c, d, a, b
  pop b, a

  lda [DATA]
  add16 h, l, c, d
  lw z
  pop d, c
  ret

; sets a character
; a: x
; b: y
; c: char
term_set:
  push d, c

  ; c, d <- offset
  push a, b
  mw a, 0
  mw c, TERM_WIDTH
  call [mul16_8]
  mw16 c, d, a, b
  pop b, a

  lda [DATA]
  add16 h, l, c, d
  pop c, d
  sw c
  ret

; draw character
; a: x
; b: y
term_draw_char:
  push a, c, d, z

  ; z <- character
  call [term_lookup]

  ; c <- x = a + 1
  mw c, a
  inc c

  ; d <- y = (b * 8)
  mw a, 8
  call [mul]
  mw d, a

  mw a, z
  call [draw_char]

  pop z, d, c, a
  ret

; draw terminal line
; a: line
term_draw_line:
  push a, b
  mw b, a                   ; b <- y
  mw a, (TERM_WIDTH - 1)    ; a <- x
.loop:
  call [term_draw_char]
  dec a
  jnz a, [.loop]
.done:
  pop b, a
  ret

; clears terminal
term_clear:
  pusha
  mw z, 0

  ; clear data
  lda a, b, [DATA]
  mw16 c, d, DATA_SIZE
  call [memset]

  ; clear screen
  lda a, b, [ADDR_BANK]
  mw16 c, d, SCREEN_SIZE_BYTES
  call [memset]

  ; reset cursor
  sw [CURSOR_X], 0
  sw [CURSOR_Y], 0

  popa
  ret

; scrolls the terminal up one line
term_scroll:
  pusha

  mw z, 0

  ; scroll data
  lda a, b, [DATA]
  lda c, d, [(DATA + TERM_WIDTH)]
  sw16 [D0], (DATA_SIZE - TERM_WIDTH)
  call [memcpy16]

  ; clear last line
  lda a, b, [(DATA + DATA_SIZE - TERM_WIDTH)]
  mw16 c, d, (TERM_WIDTH)
  call [memset]

  ; scroll graphics
  lda a, b, [ADDR_BANK]
  lda c, d, [(ADDR_BANK + (SCANLINE_WIDTH_BYTES * 8))]
  sw16 [D0], (SCREEN_SIZE_BYTES - (SCANLINE_WIDTH_BYTES * 8))
  call [memcpy16]

  ; clear last line
  lda a, b, [(ADDR_BANK + SCREEN_SIZE_BYTES - (SCANLINE_WIDTH_BYTES * 8))]
  mw16 c, d, (SCANLINE_WIDTH_BYTES * 8)
  call [memset]

  popa
  ret

; sets cursor to selected byte
; a: byte
term_set_cursor:
  pusha

  ; [SP] <- cursor byte
  push a

  ; a, b <- line offset
  mw a, 0
  lw b, [CURSOR_Y]
  mw16 c, d, (SCANLINE_WIDTH_BYTES * 8)
  call [mul16]

  ; a, b <- byte offset
  lw c, [CURSOR_X]
  add c, (SCANLINE_OFFSET_BYTES + 1)
  add16 a, b, c

  ; a, b <- starting byte
  lda [ADDR_BANK]
  add16 a, b, h, l

  ; c <- cursor byte
  pop c

  ; draw block
  mw z, 7
.loop:
  sw a, b, c
  add16 a, b, SCANLINE_WIDTH_BYTES
  dec z
  jnz z, [.loop]

  popa
  ret

; prints a character at the current cursor position
; a: character
term_print:
  push a, b, c

  mw c, a
  lw a, [CURSOR_X]
  lw b, [CURSOR_Y]

  ; is the character a newline?
  jne c, '\n', [.no_newline]

  ; clear cursor
  push a
  mw a, 0x00
  call [term_set_cursor]
  pop a

  jmp [.inc_y]
.no_newline:

  ; set/draw character
  call [term_set]
  call [term_draw_char]

  ; increment cursor
  inc a
  jne a, TERM_WIDTH, [.cont]
.inc_y:
  mw a, 0
  inc b
.cont:
  ; check if terminal needs to scroll
  jne b, TERM_HEIGHT, [.no_scroll]
  call [term_scroll]
  mw a, 0
  mw b, (TERM_HEIGHT - 1)
.no_scroll:
  sw [CURSOR_X], a
  sw [CURSOR_Y], b
  pop c, b

  ; redraw cursor
  mw a, 0xFF
  call [term_set_cursor]

  pop a
  ret

; prints a string to the terminal
; a, b: string
term_print_str:
  push a, b, c
.loop:
  lda a, b
  lw c
  jz c, [.done]
  push a
  mw a, c
  call [term_print]
  pop a
  inc16 a, b
  jmp [.loop]
.done:
  pop c, b, a
  ret
