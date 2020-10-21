.include "syscall.asm"

.data PROMPT "Please enter some text and then press return:\n"
.reserve STRING 256

    pushiw PROMPT
    pullx
    syscall PRINT
loop:
    pushiw STRING       ; Put the target string address into X
    pullx
    jsr get_string
    syscall PRINT
    b loop

.include "get_string_include.asm"