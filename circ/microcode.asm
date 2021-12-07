; JDH-8 MICROCODE
; FOR BOTH LOGISIM AND PHYSICAL CIRCUIT

@microcode

mw:
.const:
  eop1, ~sel, lreg
.reg:
  ereg, sel, ldy
  ealu, ~sel, lreg

lw:
.const:
  aimm, ~sel, emem, lreg
.reg:
  ahl, ~sel, emem, lreg

sw:
.const:
  aimm, ~sel, ereg, lmem
.reg:
  ahl, ~sel, ereg, lmem

push:
.const:
  asp, eop1, lmem
  spdec
.reg:
  asp, ~sel, ereg, lmem
  spdec

pop:
.const:
.reg:
    spinc
    asp, ~sel, emem, lreg

lda:
.const:
    eop1, ldl
    eop2, ldh
.reg:

jnz:
.const:
    eop1, jnz
.reg:
    ~sel, ereg, jnz

inb:
.const:
    eop1, lprt
    edev, ~sel, lreg
.reg:
    sel, ereg, lprt
    edev, ~sel, lreg

outb:
.const:
    eop1, lprt
    ~sel, ereg, ldev
.reg:
    sel, ereg, lprt
    ~sel, ereg, ldev

cmp:
.const:
    ~sel, ereg, ldx
    eop1, ldy
    ldf
.reg:
    ~sel, ereg, ldx
    sel, ereg, ldy
    ldf

@macro
a_const:
    ~sel, ereg, ldx
    eop1, ldy
    ~sel, ealu, lreg

@macro
a_reg:
    ~sel, ereg, ldx
    sel, ereg, ldy
    ~sel, ealu, lreg

add:
.const:
    a_const
    ldf
.reg:
    a_reg
    ldf

adc:
.const:
    a_const
    ldf
.reg:
    a_reg
    ldf

and:
.const:
    a_const
.reg:
    a_reg

or:
.const:
    a_const
.reg:
    a_reg

nor:
.const:
    a_const
.reg:
    a_reg

sbb:
.const:
    a_const
    ldf
.reg:
    a_reg
    ldf
