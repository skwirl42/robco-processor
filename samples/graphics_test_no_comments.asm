.include "syscall.asm"
.reserve GRAPHICS_START 38400
.data GRAPHICS_END 0x00
.data ERROR_STRING "Failed to set the graphics mode\n"

start:
	pushiw GRAPHICS_START
	pullx
	pushi 0x47				
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
