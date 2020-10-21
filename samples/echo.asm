.include "syscall.asm"

.data ECHO_TEXT "Testing out echo functionality:\n"
.data NON_PRINTABLE "Non-printable key pressed"
.data EMPTY_LINE "                                                            "

    syscall CLEAR           ; Clear the screen
    pushiw ECHO_TEXT
    pullx
    jsr print_string
loop:
    pushi 1         ; Call GETCH with blocking mode on
    syscall GETCH
    pushi 0         ; Compare the MSB of the returned character with 0
    cmp
    beq print_char  ; If it's 0, then the next byte is an ASCII character, so print it
    pop             ; Drop the byte, since it's not ASCII
    pushiw NON_PRINTABLE
    pullx
    jsr print_string_at_bottom_and_go_back
    b loop
print_char:
    syscall SETCH
    pushiw EMPTY_LINE
    pullx
    jsr print_string_at_bottom_and_go_back
    b loop

.include "print_string_include.asm"