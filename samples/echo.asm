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

; Expects X to be set to the beginning of the string
print_string:
    pushi 0                 ; Initialize DP and [DP] to 0
    pulldp
    pushi 0
    pull [dp]
str_loop:
    push [x+]               ; Push the string byte at X, then increment X by 1
    dup                     ; Dup the string byte
    pushi 0                 ; Compare it to 0
    cmp
    be str_print            ; Branch if eq
    push [dp]               ; Increment the value at DP
    inc
    pull [dp]
    b str_loop              ; Loop

str_print:
    pop                     ; Drop the string's null terminator
    push [dp]               ; Push the value at DP (currently the string length)
    pushi 0                 ; Add a 0 byte, to make a full word out of [DP]
    syscall PRINT           ; Print the string
    rts
