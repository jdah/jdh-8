@include "os/arch.asm"
@include "os/oscall.asm"

@org ADDR_RAM

sw [ADDR_MB], 1
jmp [main]

main:
  call [term_clear]

  mw z, 24
.loop0:
  lda a, b, [string]
  call [term_print_str]
  dec z
  jnz z, [.loop0]
.loop:
  jmp [.loop]

string:
  @ds "ABCDEFGHIJKLMNOPQRSTUVWXYZ\nZLKD\n"
