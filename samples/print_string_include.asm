; Expects X to be set to the beginning of the string
; Include this after your own code to ensure that your code 
; is executed first.
; Cursor X and Y are on the stack, string is at X
print_string_at:
    syscall SETCURSOR
    pop
    syscall PRINT
    rts

; Call with X pointing at string
print_string_at_bottom_and_go_back:
    syscall GETCURSOR
    pushiw 0
    pushiw 23
    jsr print_string_at
    syscall SETCURSOR
    rts