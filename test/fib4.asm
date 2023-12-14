# Compute first twelve Fibonacci numbers and put in array
main: la   $t0, Arr         # load address of array                          M lui addiu 
      la   $t5, N           # load address of size variable                  lui addiu
      lw   $t5, 0($t5)      # load array size                                M lw
      li   $t2, 1           # 1 is first and second Fib. number              addiu
      sw   $t2, 0($t0)      # F[0] = 1                                       sw
      sw   $t2, 4($t0)      # F[1] = F[0] = 1                                sw
      addi $t1, $t5, -2     # Counter for loop, will execute (size-2) times  M M addi
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
