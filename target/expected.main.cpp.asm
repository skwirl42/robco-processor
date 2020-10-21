; This has gotta get labelled something else
; gonna need to set some standards
.data _string "Hello, world!\n"

__start:
    jmp _main

_print_string:
    ; address of string is on stack
    dupw                    ; - keep a copy of the string address
    pullx                   ; const char *x = string;
L0_print_string:
    push [x]                ; while (*x != 0)
    cmp
    beq L1_print_string     ; {
    push [x]                ; push_byte(*x);
    pushx                   ; x++;
    incw
    pullx
    b L0_print_string       ; }
L1_print_string:
    pushx                   ; push_word(x - string)
    swapw
    subw
    pushx
    syscall 0x0102          ; syscall(0x0102) - or in decimal rather than hex
    rts

_main:
    ; result = 0[DP] where DP == 0
    pushw 5                 ; 5 + 3;
    pushw 3
    addw
    push 0                  ; int result = 
    pulldp
    pullw [dp]              
    pushw [dp]              ; - save the value of [DP] on the stack
    pushw _string           ; print_str(_string);
    jsr _print_str
    push 0                  ; - restore [DP]
    pulldp
    pullw [dp]
    pushw [dp]              ; return result;
    rts
