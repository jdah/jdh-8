; JDH-8 MICROCODE V0.1
; FOR LOGISIM CIRCUIT, NON-74

@microcode

mw:
.const:
  eop1, ~sel, lreg
.reg:
  ereg, sel, ldx
  ealu, ~sel, lreg

lw:
.const:
  aimm, ~sel, eram, lreg
.reg:
  ahl, ~sel, eram, lreg

sw:
.const:
  aimm, ~sel, ereg, lram
.reg:
  ahl, ~sel, ereg, lram

push:
.const:
  asp, eop1, lram
  spdec
.reg:
  asp, ~sel, ereg, lram
  spdec

pop:
.const:
.reg:
    spinc
    asp, ~sel, eram, lreg

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
