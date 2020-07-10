#ifndef __SYSCALL_H__
#define __SYSCALL_H__

// System
#define			EXIT			0x0000
#define			GETERROR		0x0011

// Text display
#define			GETCH			0x0100
#define			SETCH			0x0101
#define			PRINT			0x0102
#define			SETCURSOR		0x0103
#define			SETATTR			0x0104
#define			SETATTRC		0x0105
#define			CLEAR			0x0106

// Holotape access
#define			HOLOTAPECHECK   0x0200
#define			HOLOTAPEEJECT   0x0201
#define			REWIND          0x0202
#define			FIND            0x0203
#define			EXECUTE         0x0204
#define			SEEK            0x0205
#define			LOAD            0x0206
#define			LOADX           0x0207

// Mainframe communication
#define			SENDCH          0x0300
#define			RECEIVECH       0x0301
#define			SETCOMM         0x0302
#define			GETCOMM         0x0303

// Graphics display
#define			GRAPHICSTART    0x0400
#define			GRAPHICEND      0x0401

#endif // __SYSCALL_H__