main: la    $t0, DataStore          # Load the location of 0xabcdef
      lw    $t1, 0($t0)             # Load 0xabcdef into $t1
      sw    $t1, 4($t0)             # Store 0xabcdef into the next word (shouldn't stall?)
      lw    $t7, 8($t0)             # Load 0x0 into $t7
      sw    $t1, 4($t7)             # Overwrite instruction 2 with 0xabcdef (should stall?)
      lw    $t1, 0($t0)             # Load 0xabcdef into $t1
      add   $t1, $t1, $t1           # Double $t1 (should stall?)
      sw    $t1, 12($t0)            # Store it to check that it doubled correctly
      lhu   $t1, 0($t0)             # Load 0xcdef into $t1
      ori   $t2, $t1, 0             # or 0xcdef with 0, should stall?
      nop                           # nops inserted to not have icache misses
      nop
      lhu   $t1, 0($t0)             # Load 0xcdef into $t1
      ori   $t1, $t2, 0             # or 0xcdef with 0, shouldn't stall?
      la    $t0, DataStore2 
      nop
      nop
      nop
      nop
      lbu   $t1, 0($t0)             # load 0x00000100 into $t1
      lw    $t1, 0($t0)             # Shouldn't stall?
      lhu   $t0, 0($t1)             # Should stall?, ALU has to calculate 0 + $t1 which isn't ready yet
      .word 0xfeedfeed
           .word 0x00000000
DataStore: .word 0x00abcdef
           .word 0x00000000
           .word 0x00000000
           .word 0x00000000
           .word 0x00000000
           .word 0x00000000
           .word 0x00000000
           .word 0x00000000
           .word 0x00000000
           .word 0x00000000
DataStore2:.word 0x00000100