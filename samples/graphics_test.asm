.include "syscall.asm"
.reserve GRAPHICS_START 38400
.data GRAPHICS_END 0x00
.data ERROR_STRING "Failed to set the graphics mode\n"

start:
	pushiw GRAPHICS_START
	pullx
	;pushi 0x97				; 480x320, 8 bpp, border on - 153,600 bytes, should always fail
	;pushi 0x95				; 480x320, 4 bpp, border on - 76,800 bytes, should always fail
	;pushi 0x93				; 480x320, 2 bpp, border on - 38,400 bytes, should fit if GRAPHICS_START is low enough
	;pushi 0x91				; 480x320, 1 bpp, border on - 19,200 bytes, should fit easily
	;pushi 0x77				; 320x240, 8 bpp, border on - 76,800 bytes, should always fail
	;pushi 0x65				; 320x240, 4 bpp, border off - 38,400 bytes
	;pushi 0x73				; 320x240, 2 bpp, border on - 19,200 bytes
	;pushi 0x71				; 320x240, 1 bpp, border on - 9,600 bytes
	pushi 0x47				; 240x160, 8 bpp, border off - 38,400 bytes
	;pushi 0x45				; 240x160, 4 bpp, border off - 19,200 bytes
	;pushi 0x43				; 240x160, 2 bpp, border off - 9,600 bytes
	;pushi 0x41				; 240x160, 1 bpp, border off - 4,800 bytes
	;pushi 0x37				; 192x128, 8 bpp, border on - 24,576 bytes
	;pushi 0x35				; 192x128, 4 bpp, border on - 12,288 bytes
	;pushi 0x33				; 192x128, 2 bpp, border on - 6,144 bytes
	;pushi 0x31				; 192x128, 1 bpp, border on - 3,072 bytes
	;pushi 0x17				; 120x80, 8 bpp, border on - 9,600 bytes
	;pushi 0x15				; 120x80, 4 bpp, border on - 4,800 bytes
	;pushi 0x13				; 120x80, 2 bpp, border on - 2,400 bytes
	;pushi 0x11				; 120x80, 1 bpp, border on - 1,200 bytes
	syscall GRAPHICSTART
	pushi 0
	cmp
	beq draw_start
	pushiw ERROR_STRING
	pullx
	syscall PRINT
	b end_loop
draw_start:
	pushiw GRAPHICS_START
	dupw
	pullx
	sync
	pushi 0
	pulldp
	pullw [dp]

draw_loop:
	pushw [dp]
	pushiw GRAPHICS_END
	cmpw
	beq end_loop
	pushw [dp]
	dupw
	pop
	pushi 0xAA
	add
	pull [x+]
	incw
	pullw [dp]
	b draw_loop

end_loop:
	sync
	b end_loop
