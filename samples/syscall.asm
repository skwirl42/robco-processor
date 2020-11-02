; System
.defword			EXIT			0x0000
.defword			GETERROR		0x0011

; Text display
.defword			GETCH			0x0100
.defword			SETCH			0x0101
.defword			PRINT			0x0102
.defword			SETCURSOR		0x0103
.defword			SETATTR			0x0104
.defword			SETATTRC		0x0105
.defword			CLEAR			0x0106
.defword            GETCURSOR       0x0107

; Holotape access
.defword			HOLOTAPECHECK   0x0200
.defword			HOLOTAPEEJECT   0x0201
.defword			REWIND          0x0202
.defword			FIND            0x0203
.defword			EXECUTE         0x0204
.defword			SEEK            0x0205
.defword			READ            0x0206
.defword            WRITE           0x0207
.defword            BLOCKS          0x0208
.defword            BLOCKBYTES      0x0209
.defword            APPEND          0x0210

; Mainframe communication
.defword			SENDCH          0x0300
.defword			RECEIVECH       0x0301
.defword			SETCOMM         0x0302
.defword			GETCOMM         0x0303

; Graphics display
.defword			GRAPHICSTART    0x0400
.defword			GRAPHICEND      0x0401
