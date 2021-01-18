.struct exec_header
	total_length		2
	segment_count		2
	exec_start_address	2
.endstruct

.struct exec_segment_header
	location	2
	length		2
	is_exec		1
.endstruct
