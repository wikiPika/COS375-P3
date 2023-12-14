#!/bin/bash

if [ $# -ne 1 ]
then
	echo "Usage: assemble.sh <file>"
	exit 1
fi

file_path="$1"
file_path_elf="${file_path%.*}"
file_path_elf+=".elf"
file_path_bin="${file_path%.*}"
file_path_bin+=".bin" 
file_path_elfo="${file_path%.*}"
file_path_elfo+=".txt" 

if [ ! -e "$file_path" ]
then
	echo "This file doesn't exist."
	exit 2
fi

echo ""
echo "Current file:"
echo "$file_path"
echo "ELF output:"
echo "$file_path_elf"
echo "BIN output:"
echo "$file_path_bin"
echo ""
echo "Assembling:"

# else the objdump thing will scream (not fun)
mips-linux-gnu-as -mips32 $file_path -o $file_path_elf > /dev/null
mips-linux-gnu-objcopy $file_path_elf -j .text -O binary $file_path_bin > /dev/null
mips-linux-gnu-objdump -D -j .text $file_path_elf > $file_path_elfo