; Reads input until return is pressed, storing it at X, null-terminating it,
; and returning X to its original location. [DP] is used to determine if 
; characters should be echo'd as user types them
get_string:
    pushx                   ; Store the original X for later
loop:
    pushi 1                 ; Set blocking mode
    syscall GETCH           ; Grab a keypress
    pushi 0                 ; Check if printable
    cmp
    be add_char             ; Go add this character to the string
    pop                     ; Pop the non-printable character
    b loop
add_char:
    dup                     ; Check if it's a newline
    pushi 0x0D
    cmp
    be end_of_line          ; If it's a newline we'll go return the string
    push [dp]
    pushi 0
    cmp
    be commit_char
    dup
    syscall SETCH
commit_char:
    pull [x+]               ; Save the character and increment X
    b loop
end_of_line:
    push [dp]               ; If we've got echo on, echo the newline
    pushi 0
    cmp
    be finalize_line
    dup
    syscall SETCH
finalize_line:
    pull [x+]               ; Grab the newline
    pushi 0                 ; 0 terminate the string
    pull [x]
    pullx
    rts