.include "syscall.asm"

.data PROMPT "Please enter some text and then press return:\n"
.data STRING 0x00

    pushiw PROMPT
    pullx
    jsr print_string
loop:
    pushi 1             ; Set echo for get_string
    pulldp
    pushi 1
    pull [dp]
    pushiw STRING       ; Put the target string address into X
    pullx
    jsr get_string
    jsr print_string
    b loop

.include "print_string_include.asm"
.include "get_string_include.asm"