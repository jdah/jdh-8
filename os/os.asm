; JDH-8 OPERATING SYSTEM
; INCLUDED IN OSTEXT.ASM

osmain:
  sw [ADDR_MB], 1
  sw16 [ADDR_SP], 0xFEFF
  call [ADDR_RAM]
.loop:
  jmp [.loop]
