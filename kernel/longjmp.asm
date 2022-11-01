section .text
bits 64
global setjmp
;int setjmp(jmpbuf* p);
setjmp:
	push rbp		; alloc stack frame
	mov rbp,rsp
	cli				; disable interrupt while rsp change
	mov rax,[rbp+8]		;ret
	mov rsp,rdi			;jmpbuf
	add rsp,120
	lea rdi,[rbp+16]
	push rdi		; save registers
	push rax
	push r15
	push r14
	push r13
	push r12
	push r11
	push r10
	push r9
	push r8
	push rdx
	push rbx
	mov rdi,[rbp]
	push rdi
	push rsi
	push rcx
	mov rax,[rbp]
	mov [rsp+8],rax
	mov eax,0
	leave			; conveniently switch rsp back
	sti				; and reenable interrupt
	ret
global longjmp
;void longjmp(jmpbuf* p,int r);
longjmp:	;THIS MEANS NO WAY BACK
	xchg bx,bx
	cli				; also should keep interrupt out
	mov rsp,rdi
	mov eax,esi
	pop rcx
	pop rsi
	pop rbp
	pop rbx
	pop rdx
	pop r8
	pop r9
	pop r10
	pop r11
	pop r12
	pop r13
	pop r14
	pop r15
	pop rcx	;eip
	pop rsp	;esp
	sti
	jmp rcx