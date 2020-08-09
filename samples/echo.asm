.include "syscall.asm"

.data ECHO_TEXT "Testing out echo functionality:\n"

    syscall CLEAR           ; Clear the screen
    pushiw ECHO_TEXT
    pullx
    jsr print_string
loop:
    pushi 1         ; Call GETCH with blocking mode on
    syscall GETCH
    pushi 0         ; Compare the MSB of the returned character with 0
    cmp
    be print_char   ; If it's 0, then the next byte is an ASCII character, so print it
    pop             ; Drop the byte, since it's not ASCII
    b loop
print_char:
    syscall SETCH
    b loop

.include "print_string_include.asm"