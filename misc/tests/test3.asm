@org 0x0000

sw [0xFFFA], 0x01

lda [data]

lw f
sw [(0x8008 + (32 * 0))], f
add l, 1

lw f
sw [(0x8008 + (32 * 1))], f
add l, 1

lw f
sw [(0x8008 + (32 * 2))], f
add l, 1

lw f
sw [(0x8008 + (32 * 3))], f
add l, 1

lw f
sw [(0x8008 + (32 * 4))], f
add l, 1

lw f
sw [(0x8008 + (32 * 5))], f
add l, 1

lw f
sw [(0x8008 + (32 * 6))], f
add l, 1

lw f
sw [(0x8008 + (32 * 7))], f
add l, 1

lw f
sw [(0x8008 + (32 * 8))], f
add l, 1

lw f
sw [(0x8008 + (32 * 9))], f
add l, 1

lw f
sw [(0x8008 + (32 * 10))], f
add l, 1

lw f
sw [(0x8008 + (32 * 11))], f
add l, 1

lw f
sw [(0x8008 + (32 * 12))], f
add l, 1

lw f
sw [(0x8008 + (32 * 13))], f
add l, 1

lw f
sw [(0x8008 + (32 * 14))], f
add l, 1

lw f
sw [(0x8008 + (32 * 15))], f
add l, 1

lw f
sw [(0x8008 + (32 * 16))], f
add l, 1

lw f
sw [(0x8008 + (32 * 17))], f
add l, 1

lw f
sw [(0x8008 + (32 * 18))], f
add l, 1

lw f
sw [(0x8008 + (32 * 19))], f
add l, 1

lw f
sw [(0x8008 + (32 * 20))], f
add l, 1

lw f
sw [(0x8008 + (32 * 21))], f
add l, 1

lw f
sw [(0x8008 + (32 * 22))], f
add l, 1

lw f
sw [(0x8008 + (32 * 23))], f
add l, 1

lw f
sw [(0x8008 + (32 * 24))], f
add l, 1

done:
  jmp [done]

data:
  @db 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00
  @db 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00
  @db 0x00, 0x66, 0x66, 0x00, 0x81, 0xE7, 0x3C, 0x00
