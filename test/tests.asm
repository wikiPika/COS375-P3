# instruction testing
# note: using $t8 as memory location
main:
    la      $t8,    384                 # output starts at this address (word = addr / 4)

    li      $t0,    4                   # $t0 = 4
    li      $t1,    5                   # $t1 = 5
    add     $t9,    $t0,        $t1     # $t9 = 4 + 5
    jal     output

    addi    $t2,    $t1,        3       # $t2 = $t1 + 3 = 8
    addi    $t9,    $t2,        0       # $t9 = $t2 = 8
    jal     output

    li      $t0,    3                   # $t0 = 0b0011
    li      $t1,    2                   # $t1 = 0b0010
    and     $t9,    $t0,        $t1     # $t9 = $t0 & $t1 = 0b0010
    jal     output

    andi    $t9,    $t0,        2       # output should be the same
    jal     output

    li      $t0,    4                   # $t0 = 0b0100
    li      $t1,    7                   # $t1 = 0b0111
    nor     $t9,    $t0,        $t1     # $t9 = ~($t0 | $t1) = 0b1.....1000
    jal     output

    li      $t0,    9                   # $t0 = 0b1001
    li      $t1,    6                   # $t1 = 0b0110
    or      $t9,    $t0,        $t1     # $t9 = $t0 | $t1 = 0b1111
    jal     output

    ori     $t9,    $t0,        6       # same output
    jal     output

    li      $t0,    3                   # $t0 = 3
    sll     $t9,    $t0,        3       # $t9 = $t0 << 3 = 24
    jal     output

    li      $t0,    16                  # $t0 = 16
    srl     $t9,    $t0,        3       # $t9 = $t0 >> 3 = 2
    jal     output

    li      $t0,    11                  # $t0 = 11
    li      $t1,    8                   # $t1 = 8
    sub     $t9,    $t0,        $t1     # $t9 = $t0 - $t1
    jal     output

    jal     output                      # output some zeroes to distinguish
    jal     output                      # math section and branching section

    beq     $t0,    $t1,        breq    # $t0 = 11
    bne     $t0,    $t1,        brne    # $t1 = 8

breq:
    li      $t9,    213213
    jal     output

brne:
    li      $t9,    32                  # intended output
    jal     output

    li      $t0,    -3
    bgtz    $t0,    brgtz
    blez    $t0,    brlez

brgtz:
    li      $t9,    213213
    jal     output

brlez:
    li      $t9,    64                  # intended output
    jal     output

    li      $t0,    65535               # 0x11111111 11111111
    sll     $t0,    16                  # 0x11111111 11111111 00000000 00000000

    li      $t1,    65535
    add     $t0,    $t0,        $t1     # $t0 = MAX_VALUE

    li      $t1,    0                   # $t1 = 0

    lbu     $t9,    0($zero)            # M[0] = 0x24180180
    jal     output  # should only store 0x24

    lhu     $t9,    0($zero)            # $t0 = 0x11111111 11111111 00000000 00000000
    jal     output # should only store 0x2418

    lui $t9, 123
    jal output 

    li $t0, 4
    li $t1, 3

    slt     $t9,    $t0,        $t1     # $t9 = 0 because 4 > 3
    jal     output

    slti    $t9,    $t0,        8      # $t9 = 1 because 4 < 8
    jal     output

    li      $t0,    65535               # 0x11111111 11111111
    sll     $t0,    16                  # 0x11111111 11111111 00000000 00000000

    li      $t1,    65535
    add     $t0,    $t0,        $t1     # $t0 = MAX_VALUE

    addi $t9, $t0, 0
    jal output

    sb      $t0,    0($t8)              # store 0x11111111
    addi    $t8,    $t8,        4

    sh      $t0,    0($t8)              # store 0x11111111 11111111
    addi    $t8,    $t8,        4

    j       end

# using $t8 as memory pointer for outputs
# move desired output into $t9 and then jal output
# then clear $t9
output:
    sw      $t9,    0($t8)              # M[$t8] = $t9
    addi    $t8,    $t8,        4       # t8++
    li      $t9,    0                   # reset $t9
    jr      $ra                         # return

end:
.word 0xfeedfeed                        # end of program, start of memory