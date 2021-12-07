@org 0x0000

@include "os/arch.asm"
@include "os/ostest.asm"

jmp [ostest]

osmain:
  jmp [osmain]
