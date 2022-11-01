section .text
bits 64

KERNEL_BASE equ 0xffffff80_00000000
cpus equ KERNEL_BASE+0x59000

global apmain_asm
extern ncpu, apmain
apmain_asm:
    xor rax, rax
    mov al, byte [rel ncpu]
    mov rdi, cpus
    lea rbx, [rax+rax*8]
    shl rbx, 4
    add rdi, rbx  ; cpus + ncpu*0x90
    
    xor rcx, rcx
    mov cl, [rdi+0x6c]  ; index
    shl rcx, 11     ; 0x800 (2k) stack per cpu
    mov rbx, KERNEL_BASE+0x5a000
    lea rsp, [rbx+rcx]

    call apmain