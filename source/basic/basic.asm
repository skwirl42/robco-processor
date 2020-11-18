; Main entry point for RobCo BASIC
.include "syscall.asm"

.data               PROMPT              "OK\n"
.reserve            INPUT_BUFFER        256

.defbyte			cr				    0x0D
.defword            INPUT_BUFFER_SIZE   256

start:
    syscall CLEAR
    pushiw PROMPT
    syscall PRINT
    b read_line

read_line:
    pushiw INPUT_BUFFER
    pullx
    jsr get_string
    
execute_line:
    b parse_line

parse_line:
    
    
print_prompt:
    pushiw PROMPT
    syscall PRINT


.include "get_string_include.asm"
.include "parse_int_include.asm"