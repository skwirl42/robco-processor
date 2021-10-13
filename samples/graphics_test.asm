.include "syscall.asm"
.reserve GRAPHICS_START 38400
.data GRAPHICS_END 0x00
.data ERROR_STRING "Failed to set the graphics mode\n"

start:
	pushiw GRAPHICS_START
	pullx
	;pushi 0x3C				; 480x320, 8 bpp, border on - 153,600 bytes, should always fail
	;pushi 0x2C				; 480x320, 4 bpp, border on - 76,800 bytes, should always fail
	;pushi 0x1C				; 480x320, 2 bpp, border on - 38,400 bytes, should fit if GRAPHICS_START is low enough
	;pushi 0x0C				; 480x320, 1 bpp, border on - 19,200 bytes, should fit easily
	;pushi 0x3B				; 320x240, 8 bpp, border on - 76,800 bytes, should always fail
	pushi 0x23				; 320x240, 4 bpp, border off - 38,400 bytes
	;pushi 0x1B				; 320x240, 2 bpp, border on - 19,200 bytes
	;pushi 0x0B				; 320x240, 1 bpp, border on - 9,600 bytes
	;pushi 0x32				; 240x160, 8 bpp, border off - 38,400 bytes
	;pushi 0x22				; 240x160, 4 bpp, border off - 19,200 bytes
	;pushi 0x12				; 240x160, 2 bpp, border off - 9,600 bytes
	;pushi 0x02				; 240x160, 1 bpp, border off - 4,800 bytes
	;pushi 0x39				; 192x128, 8 bpp, border on - 24,576 bytes
	;pushi 0x29				; 192x128, 4 bpp, border on - 12,288 bytes
	;pushi 0x19				; 192x128, 2 bpp, border on - 6,144 bytes
	;pushi 0x09				; 192x128, 1 bpp, border on - 3,072 bytes
	;pushi 0x38				; 120x80, 8 bpp, border on - 9,600 bytes
	;pushi 0x28				; 120x80, 4 bpp, border on - 4,800 bytes
	;pushi 0x18				; 120x80, 2 bpp, border on - 2,400 bytes
	;pushi 0x08				; 120x80, 1 bpp, border on - 1,200 bytes
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

; Pseudocode:
;start()
;{
;	result = start_graphics(GRAPHICS_START, 0x47);			// syscall GRAPHICSTART
;	if (result == 0)
;	{
;		x = GRAPHICS_START;
;		sync();
;		dp = 0;
;		direct_page[dp] = GRAPHICS_START;
;		while (dp != GRAPHICS_END)
;		{
;			// this is a bit harder to turn into pseudocode
;			GRAPHICS_START[x++] = (direct_page[dp] >> 8) << 8 + 0xAA;
;			direct_page[dp] = direct_page[dp] + 1;
;		}
;
;		for(;;)
;		{
;			sync();
;		}
;	}
;	else
;	{
;		print("Failed to set the graphics mode\n");
;	}
;}
