.include "syscall.asm"

.data ERROR_STRING "Failed to set the graphics mode\n"
.data GRAPHICS_START 0x00
;.defword GRAPHICS_START 0x0100
.defword GRAPHICS_TEST_AREA 0x6000

start:
	pushiw GRAPHICS_START
	pullx
	;pushi 0x97				; 480x320, 8 bpp, border on
	;pushi 0x91				; 480x320, 1 bpp, border on
	;pushi 0x37				; 192x128, 8 bpp, border on
	;pushi 0x35				; 192x128, 4 bpp, border on
	;pushi 0x33				; 192x128, 2 bpp, border on
	;pushi 0x31				; 192x128, 1 bpp, border on
	pushi 0x17				; 120x80, 8 bpp, border on
	;pushi 0x15				; 120x80, 4 bpp, border on
	;pushi 0x13				; 120x80, 2 bpp, border on
	;pushi 0x11				; 120x80, 1 bpp, border on
	syscall GRAPHICSTART
	pushi 0
	cmp
	be draw_start
	pushiw ERROR_STRING
	pullx
	jsr print_string
	b loop
draw_start:
	pushiw 0
	dup
	pulldp
	pullw [dp]

draw_loop:
	pushw [dp]
	pushiw GRAPHICS_TEST_AREA
	cmpw
	be get_up
	pushw [dp]
	dupw
	pop
	pull [x+]
	incw
	pullw [dp]
	sync
	b draw_loop

get_up:
loop:
	sync
	b loop

.include "print_string_include.asm"