# 6502 Emulator

Single file 6502 machine language emulator used in the NES (Nintendo Enterntainment System).
Emulator is able to process 6502 machine language instructions. Other parts like graphics, controls 
or sound are not included. Does not support unofficial instructions. Also features the famous JMP-indirect bug ($6C).
If processor reads low byte of the target address ending to FF, the high byte will be read from the beginning of the same memory page.
Code is ugly, but works so, I don't care.

Emulator passes `nestest`

## Files

main.c
cpu.c
cpu.h
nestest.nes
nestest.log

# Build

gcc main.c cpu.c -o main

# Run

./main

## References

https://www.nesdev.org/wiki/Nesdev_Wiki


