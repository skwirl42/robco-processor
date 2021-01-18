.include "syscall.asm"
.include "holotape.asm"
.include "exec_file.asm"

.reserve BLOCK_BUFFER 0x400
.data NO_TAPE_STRING "There's no tape in the deck\n"
.data READ_FAILED_STRING "Failed to read from the tape\n"
.data FILENAME_PRINT "Filename: "

start:
	syscall CLEAR
	syscall HOLOTAPECHECK
	pushi 0x01
	cmp
	beq read_block
	pushiw NO_TAPE_STRING
	pullx
	syscall PRINT
	syscall EXIT

read_block:
	pushiw BLOCK_BUFFER
	pullx
	syscall READ
	pushi 0x00
	cmp
	beq read_success
	pushiw READ_FAILED_STRING
	pullx
	syscall PRINT
	syscall EXIT

read_success:
	pushi 0								; set DP to the beginning of the direct page
	pulldp
	pushiw BLOCK_BUFFER					; put the filename's location into X
	pushiw holotape_block.filename
	addw
	pullx
prepare_filename_loop:
	push [x+]							; push the current character of the string
	dup									; dup it
	pull [dp+]							; pull the character into the copied string
	pushdp								; push the value of DP
	pushi HOLO_FILENAME_SIZE			; is it at the end of the string's max length?
	cmp
	beq print_filename
	pushi 0								; the current character will be on the stack
	cmp									; are we at the end of the filename?
	beq print_filename
	b prepare_filename_loop
print_filename:
	pushiw FILENAME_PRINT				; print out "Filename: "
	pullx
	syscall PRINT
	pushi 0								; 0-terminate the copied string
	pull [dp]
	pushiw 0							; print the copied filename
	pullx
	syscall PRINT
	syscall EXIT

; Pseudocode:
;start()
;{
;	clear();											// syscall CLEAR
;	result = check_for_holotape();						// syscall HOLOTAPE_CHECK
;	if (result)
;	{
;		result = read_holotape_block();					// syscall READ
;		if (result == 0) // success
;		{
;			dp = 0;
;			x = BLOCK_BUFFER + holotape_block.filename;
;			do
;			{
;				direct_page[dp++] = BLOCK_BUFFER.filename[x++];
;			} while(dp != HOLO_FILENAME_SIZE && direct_page[dp] != 0);
;			
;			print("Filename: ");						// syscall PRINT
;			direct_page[dp] = 0;
;			x = 0;
;			print(memory[x]);							// syscall PRINT
;			exit();										// syscall EXIT
;		}
;		else
;		{
;			print("Failed to read from the tape\n");	// syscall PRINT
;			exit();										// syscall EXIT
;		}
;	}
;	else
;	{
;		print("There's no tape in the deck\n");			// syscall PRINT
;		exit();											// syscall EXIT
;	}
;}
