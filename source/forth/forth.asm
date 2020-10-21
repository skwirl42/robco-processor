; Main entry point for the forth repl
.include "syscall.asm"

.data               PROMPT              "ok> "
.reserve            INPUT_BUFFER        256

.defbyte			cr				    0x0D
.defbyte            space               0x20
.defword            INPUT_BUFFER_SIZE   256

start:
    syscall CLEAR

read_line:
    pushiw INPUT_BUFFER
    pullx
read_line_loop:
    pushi 1                 ; Set blocking mode
    syscall GETCH           ; Grab a keypress
    dup                     ; Duplicate the high byte of the key to see if it's a non-printable
    pushi 0
    cmp
    be read_line_printable

read_line_unprintable:


read_line_printable:
    drop                    ; Drop the high byte, since this we want only the low byte
    dup
    pushi cr
    cmp
    be read_line_enter
    

read_line_enter:
    pushi 0                 ; End the string and return
    pull [x]
    rts

.include "print_string_include.asm"