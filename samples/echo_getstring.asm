.include "syscall.asm"

.data PROMPT "Please enter some text and then press return:\n"
.reserve STRING 256

loop:
    pushiw PROMPT
    pullx
    syscall PRINT
    pushiw STRING       ; Put the target string address into X
    pullx
    jsr get_string
    syscall PRINT
    pushi 0x0D
    syscall SETCH
    b loop

.include "get_string_include.asm"