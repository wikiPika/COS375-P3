# Compute first twelve Fibonacci numbers and put in array
main: la   $t0, Arr         # load address of array
      la   $t5, N           # load address of size variable
      lw   $t5, 0($t5)      # load array size
      .word 0xfeedfeed
Arr:  .word 0x0             # size of "array"
      .word 0x0             # size of "array"
      .word 0x0             # size of "array"
      .word 0x0             # size of "array"
      .word 0x0             # size of "array"
      .word 0x0             # size of "array"
      .word 0x0             # size of "array"
      .word 0x0             # size of "array"
      .word 0x0             # size of "array"
      .word 0x0             # size of "array"
      .word 0x0             # size of "array"
      .word 0x0             # size of "array"
N:    .word 0xc             # size of "array"
