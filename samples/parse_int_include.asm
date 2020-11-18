.defbyte PARSE_INT_DP_WORKING_AREA      0x30
.defbyte PARSE_INT_NEGATIVE_FLAG_INDEX  0
.defbyte PARSE_INT_INTERMEDIARY_INDEX   1

; On enter: X points at a string
; On exit:  X points at first char after int
;           [DP] contains the parsed number
parse_int:

parse_int_init:
    pushi PARSE_INT_DP_WORKING_AREA         ; Set up working area in the direct page
    pushi PARSE_INT_INTERMEDIARY_INDEX      ; Clear the intermediary result
    add
    pulldp
    pushiw 0
    pullw [dp]
    pushi PARSE_INT_DP_WORKING_AREA
    pushi PARSE_INT_NEGATIVE_FLAG_INDEX     ; Clear the negative flag
    add
    pulldp
    pushi 0
    pull [dp]

skip_spaces:
    push [x+]
    pushi 0x20
    cmp
    be skip_spaces
    pushx
    decw
    pullx

parse_sign:
    push [x]
    dup
    pushi '-'
    cmp
    be handle_negative
    dup
    pushi '+'
    cmp
    be handle_positive
    b parse_digit

handle_negative:
    pushi 1                         ; Set the negative flag
    pull [dp]
handle_positive:
    pop                             ; Discard the +/- from the stack
    pushx                           ; Increment X
    incw
    pullx
    push [x]                        ; Put the next character up for processing

parse_digit:
    pushi PARSE_INT_DP_WORKING_AREA
    pushi PARSE_INT_INTERMEDIARY_INDEX
    add
    pulldp
    pushw [dp]
    dupw
    pushi 3                         ; intermediary << 3
    shlw
    swapw
    pushi 1                         ; intermediary << 1
    shlw
    addw                            ; Stack is now intermediary * 10, char
    pushi 3
    roll                            ; Pull the char to the front
    pushi 0
    swap                            ; Stack is now char, 0, intermediary[lsb], intermediary[msb]
    pushi 0
    pushi '0'
    subw
    addw                            ; Stack is now the updated intermediary value
    pullw [dp]

get_digit:
    push [+x]
    dup
    pushi 0
    cmp
    be finalize
    b parse_digit

finalize:


end:
    rts
