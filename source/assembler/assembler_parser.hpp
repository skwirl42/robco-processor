#pragma once

// \s refers to non-end-of-line characters

// End-of-line <eol>
// (<cr>|<lf>|<eof>)+

// Hex byte literal
// 0(x|X)<hex digit>{1,2}

// Hex integer literal
// 0(x|X)<hex digit>{1,4}

// Byte sequence - note: the whitespace can include <eol> characters, as long as each line begins with other white space
// <hex byte literal>[\s+<hex byte literal>]+<eol>

// Instruction line
// \s+<instruction>[\s+(<symbol>|<single-value literal>|<register index>)]<eol>

// Byte/word definition
// .<def directive>\s+<symbol>\s+<integer literal><eol>

// Data definition
// .data\s+[<symbol>\s+](<quoted string>|<byte sequence>)

// Reservation
// .reserve\s+<symbol>\s+<integer literal><eol>

// Include directive
// .include\s+<quoted filename><eol>

// Org directive
// .org\s+<integer literal><eol>

// Register index - pre-increment/decrement
// [\s*(+|-){0,2}\s*<register>\s*]
// - post-increment/decrement
// [\s*<register>\s*(+|-){1,2}\s*]

// Comment
// ;[<characters except EOL>]*<eol>
