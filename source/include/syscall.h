#ifndef __SYSCALL_H__
#define __SYSCALL_H__

// System
#define         SYSCALL_NONE            0x0000
#define			SYSCALL_EXIT			0x0001
#define			SYSCALL_GETERROR		0x0011

// Text display
#define			SYSCALL_GETCH			0x0100
#define			SYSCALL_SETCH			0x0101
#define			SYSCALL_PRINT			0x0102
#define			SYSCALL_SETCURSOR		0x0103
#define			SYSCALL_SETATTR			0x0104
#define			SYSCALL_SETATTRC		0x0105
#define			SYSCALL_CLEAR			0x0106
#define         SYSCALL_GETCURSOR       0x0107

// Holotape access
#define			SYSCALL_HOLOTAPECHECK   0x0200
#define			SYSCALL_HOLOTAPEEJECT   0x0201
#define			SYSCALL_REWIND          0x0202
#define			SYSCALL_FIND            0x0203
#define			SYSCALL_EXECUTE         0x0204
#define			SYSCALL_SEEK            0x0205
#define			SYSCALL_LOAD            0x0206
#define			SYSCALL_LOADX           0x0207

// Mainframe communication
#define			SYSCALL_SENDCH          0x0300
#define			SYSCALL_RECEIVECH       0x0301
#define			SYSCALL_SETCOMM         0x0302
#define			SYSCALL_GETCOMM         0x0303

// Graphics display
#define			SYSCALL_GRAPHICSTART    0x0400
#define			SYSCALL_GRAPHICEND      0x0401

#endif // __SYSCALL_H__
