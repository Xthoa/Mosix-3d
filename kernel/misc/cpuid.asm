bits 64
section .text

;void GetCpuid(u32* data,u32 eax);
global GetCpuid
GetCpuid:
	push rbx
	push rbp
	mov rbp,rdi
	mov rax,rsi
	cpuid
	mov [rbp],eax
	mov [rbp+4],ebx
	mov [rbp+8],ecx
	mov [rbp+12],edx
	pop rbp
	pop rbx
	ret