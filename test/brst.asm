.set noreorder
main:   la      $s0, arr
        addi    $t1, $zero, 10
        addi    $t2, $zero, -10
        addi    $t3, $zero, 5
        beq     $t3, $t1, halt      # there should be a single cycle stall here

        addi    $t3, $t3, -15
        bne     $t3, $t2, halt      # again, single cycle stall

        srl     $t1, $t1, 1
        blez    $t1, halt           # single cycle stall

        lw      $t1, 0($s0)
        bgtz    $t1, go             # two cycle stall
        addiu   $t1, $t1, 1

        sll     $t1, $t1, 2         # these aren't executed
        sll     $t1, $t1, 4         # ---
        sll     $t1, $t1, 5         # ---

go:     srl     $t1, $t1, 5
        nop
        lw      $t2, 4($s0)
        addi    $t3, $t3, 2
        beq     $t2, $t1, halt      # one cycle stall since lw is 2 instructions ahead

        lw      $t2, 8($s0)
        addi    $t2, $t2, 2         # one cycle stall between lw-addi,
        beq     $t2, $t1, halt      # one cycle stall between addi-beq
        addi    $t3, $t3, 1
        addi    $t3, $t3, 1
        addi    $t3, $t3, 1
halt:   .word 0xfeedfeed
arr:    .word 0x000000ff
        .word 0x000000ee