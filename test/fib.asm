; TEST
; d = 21
; a = 13
; b = 21
; c = 0
; [0x0000] = 0

; calculates the 8th fibonacci number leaves the result in d

@org 0x0000
@define COUNT 7

fib:
    mw a, 0
    mw b, 1
    mw c, COUNT
.loop:
    mw d, a
    add d, b
    mw a, b
    mw b, d
    clb
    sbb c, 1
    jnz c, [.loop]
.done:
    halt

@dn 1024
