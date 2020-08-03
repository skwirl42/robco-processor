.include "syscall.asm"

.data HELLO_WORLD 0x48 0x65 0x6C 0x6C 0x6F 0x20 0x77 0x6F 0x72 0x6C 0x64 0x21 0x0A 0x00

    syscall CLEAR
start:
    pushi 0
    pulldp
    pushi 0
    add so,dp,so,so
    sub dp,dp,so,so
    pushiw HELLO_WORLD
    pullx
loop:
    pushi 0
    add so,x,so,so
    dup
    pushi 0
    cmp
    be print_str
    inc dp,so,so,so
    pushx
    incw
    pullx
    b loop

print_str:
    add so,dp,so,so
    pushi 0
    syscall PRINT

    b start
