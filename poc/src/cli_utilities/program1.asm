[BITS 64]

; Include useful functions, constants and macros
%include "../../include/global/macro.asm"

global _start

section .text

_start:
	; preserve all registers
	pushaq

	; execute program
_pre_main:
	call main - $ - 1

	; restore registers
	popaq
	ret

main:
	; preserve all registers
	pushaq

	; Syscall write
	mov	rdi,	1		; unsigned int fd -> stdout
	;	-> obtain main's RIP from stack frame
	mov	rsi,	rsp
	add	rsi,	8 * 15
	mov	rsi,	[rsi]
	;	-> substract CALL's opcode size
	sub	rsi,	5
	;	-> calculate relative (PIC) position of string to be printed
	add	rsi,	content - _pre_main
	mov	rdx,	content_length	; size_t count
	mov	rax,	1		; sys_write
	syscall

	; syscall exit (temporary)
	mov	rdi,	0		; int error_code
	mov	rax,	60		; sys_exit
	syscall

	; restore registers
	popaq
	ret

section .data

content		db	"test",CR,LF, 0
content_length	equ	$ - content
