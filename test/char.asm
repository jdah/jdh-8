; TEST
; a = 0x0C
; b = 0x5C
; c = 0x27

jmp [test]

chars:
@db '\f'
@db '\\', '\''

test:
lw a, [(chars + 0)]
lw b, [(chars + 1)]
lw c, [(chars + 2)]
halt
