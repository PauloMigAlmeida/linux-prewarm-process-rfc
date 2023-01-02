[BITS 64]

; Include useful functions, constants and macros
%include "../../include/global/macro.asm"

global _start

;section .text

_start:
	; preserve all registers
	pushaq

	; execute program
_pre_main:
	call main

	; restore registers
	popaq
	ret

main:
	; preserve all registers
	pushaq

	; Syscall write
	mov	rdi,	1			; unsigned int fd -> stdout

	mov	rsi,	rsp			; obtain main's RIP from stack frame 
	add	rsi,	8 * 15
	mov	rsi,	[rsi]
	sub	rsi,	5			; substract CALL's opcode size 
	add	rsi,	content - _pre_main	; calc relative (PIC) position of content

	mov	rdx,	content_length		; size_t count
	mov	rax,	1			; sys_write
	syscall

	; restore registers
	popaq
	ret

;section .data

content		db	"test",CR,LF, 0
content_length	equ	$ - content

; Zero-pad utility so they all have the same size
times 1024-($-$$) db 0
