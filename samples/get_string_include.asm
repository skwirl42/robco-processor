; Reads input until return is pressed, storing it at X, null-terminating it,
; and returning X to its original location.
get_string:
    pushx                   ; Store the original X for later
get_string_loop:
    pushi 1                 ; Set blocking mode
    syscall GETCH           ; Grab a keypress
    swap                    ; The LSB is on the top of the stack, and we want the MSB
    pushi 0                 ; Check if printable
    cmp
    beq gs_handle_ascii     ; Go add this character to the string
    pop                     ; Pop the non-printable character
    b get_string_loop
gs_handle_ascii:
    dup                     ; Check if it's a newline
    pushi 0x0D
    cmp
    beq gs_end_of_line      ; If it's a newline we'll go return the string
gs_handle_char:
    dup
    syscall SETCH
    pull [x+]               ; Save the character and increment X
    b get_string_loop
gs_end_of_line:
    syscall SETCH
    pushi 0                 ; 0 terminate the string
    pull [x]
    pullx                   ; Restore X
    rts
