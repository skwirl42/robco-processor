.include "syscall.asm"

; "Hello world!\n"
.data HELLO_WORLD 0x48 0x65 0x6C 0x6C 0x6F 0x20 0x77 0x6F 0x72 0x6C 0x64 0x21 0x0A 0x00

    syscall CLEAR           ; Clear the screen
start:
    pushiw HELLO_WORLD      ; Set X to the start of the string data
    pullx
    syscall PRINT           ; Print the string

    b start                 ; Do it again!
