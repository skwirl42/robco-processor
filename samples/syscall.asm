; System
.word			EXIT			0x0000
.word			GETERROR		0x0011

; Text display
.word			GETCH			0x0100
.word			SETCH			0x0101
.word			PRINT			0x0102
.word			SETCURSOR		0x0103
.word			SETATTR			0x0104
.word			SETATTRC		0x0105
.word			CLEAR			0x0106

; Holotape access
.word			HOLOTAPECHECK   0x0200
.word			HOLOTAPEEJECT   0x0201
.word			REWIND          0x0202
.word			FIND            0x0203
.word			EXECUTE         0x0204
.word			SEEK            0x0205
.word			LOAD            0x0206
.word			LOADX           0x0207

; Mainframe communication
.word			SENDCH          0x0300
.word			RECEIVECH       0x0301
.word			SETCOMM         0x0302
.word			GETCOMM         0x0303

; Graphics display
.word			GRAPHICSTART    0x0400
.word			GRAPHICEND      0x0401
