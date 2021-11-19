; JDH-8 OPERATING SYSTEM
; INCLUDED IN OSTEXT.ASM

osmain:
  sw [ADDR_SPL], 0xFF
  sw [ADDR_SPH], 0xFE
  sw [ADDR_MB], 1
  sw [(ADDR_BANK + 4)], 0xFF
  jmp [ADDR_RAM]
  call [ADDR_RAM]
.loop:
  jmp [.loop]
