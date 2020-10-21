; Without syscall.asm included all syscall lines should report bad symbols
.data HELLO_WORLD 0x48 0x65 0x6C 0x6C 0x6F 0x20 0x77 0x6F 0x72 0x6C 0x64 0x21 0x0A 0x00

    syscall CLEAR           ; Clear the screen
    pushi 0                 ; Set DP to point to the first byte of the direct page
    pulldp
start:
    pushi 0                 ; Set the value at DP to 0
    pull [dp]
    pushiw HELLO_WHIRLED    ; Set X to the start of the string data
    pullx
loop:
    push [x+]               ; Push the string byte at X, then increment X by 1
    dup                     ; Dup the string byte
    pushi 0                 ; Compare it to 0
    cmp
    beq print_string        ; Branch if eq
    push [dp]               ; Increment the value at DP
    inc
    pull [dp]
    b loop                  ; Loop

print_str:
    pop                     ; Drop the string's null terminator
    push [dp]               ; Push the value at DP (currently the string length)
    pushi 0                 ; Add a 0 byte, to make a full word out of [DP]
    syscall PRINT           ; Print the string

    b start                 ; Do it again!
