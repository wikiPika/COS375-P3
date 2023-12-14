addiu   $t3,    $zero,  2       # t3 is 2
addiu   $t4,    $zero,  0xfeed  # t4= 0xfeed
addiu   $t5,    $zero,  0x200C  # upper 16 bits of addi $t4 = $zero +5
addiu   $t6,    $zero,  0x0005  # lower 16 bits of addi $t4 = $zero +5
sh      $t5,    0x8000          # store upper bits of exception handler with instruction addi $t4 = $zero +5
sh      $t6,    0x8002          # store lower bits of exception handler
sh      $t4,    0x8004          # store 0xfeedfeed into exception handler
sh      $t4,    0x8006
ll      $t2,    0x4002          # An illegal instruction in our implementation.
addiu   $t7,    $zero,  7       # this line should not execute, t7 should not be 7
.word 0xfeedfeed

