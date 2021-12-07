; OS TEXT
; THIS IS THE MAIN ASSEMBLY FILE FOR THE JDH-8 ROM

; corresponds to OSCALL_OFFSET in oscall.asm
; TODO: jmp [ostest]
jmp [osmain]

; 5 blank bytes for empty space in jump table
; a jump to one of these will fault/restart the OS
@macro
BLANK:
  jmp [0x0000]

; DO NOT MOVE THESE CALLS WITHOUT ADJUSTING HEADER OSCALL.ASM

; === MATH.ASM ===
jmp [mul]
jmp [mul16_8]
jmp [mul16]
jmp [cmp16]
blank
blank
blank
blank

; === UTIL.ASM ===
jmp [strlen]
jmp [memcpy]
jmp [memcpy16]
jmp [memset]
blank
blank
blank
blank

; === SHIFT.ASM ===
jmp [shl]
jmp [shr]
blank
blank

; === GFX.ASM ===
jmp [set_pixel]
jmp [draw_char]
jmp [draw_str]
blank
blank
blank
blank
blank

; === TERM.ASM ===
jmp [term_print]
jmp [term_print_str]
jmp [term_clear]
blank
blank
blank
blank
blank

; ALL LIBRARIES
@include "os/arch.asm"
@include "os/lib/math.asm"
@include "os/lib/util.asm"
@include "os/lib/shift.asm"
@include "os/lib/font.asm"
@include "os/lib/gfx.asm"
@include "os/lib/term.asm"

@include "os/ostest.asm"
@include "os/os.asm"
