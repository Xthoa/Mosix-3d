bits 64
section .text

global switch_context
switch_context:  ;void switch_context(* now, * next);
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    mov [rdi + 0x08],rsp
    mov rsp,[rsi + 0x08]     ; Process* -> rsp
    mov rax,[rsi]    ; Process* -> vm
    mov rax,[rax]     ; Vmspace* -> cr3
    mov rbx,cr3
    cmp rbx,rax
    jz .1
    mov rbx, 0x1000
    cmp rbx, rax
    jz .doit
    mov rbx, rax
    shr rbx, 22
    test rbx, rbx
    jz .doit
    xchg bx,bx
    .doit:
    mov cr3,rax  ; avoid flushing TLB
    .1:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

global switch_context_exit
switch_context_exit:  ;void switch_context_exit(* now, * next, * now.stat);
    mov rsp,[rsi + 0x08]     ; Process* -> rsp
    mov rax,[rsi]    ; Process* -> vm
    mov rax,[rax]     ; Vmspace* -> cr3
    mov word [rdx], 7
    mov rbx,cr3
    cmp rbx,rax
    jz .1
    mov cr3,rax  ; avoid flushing TLB
    .1:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

global ProcessEntryStub
extern ProcessEntrySafe
ProcessEntryStub:    ; rbx=routine
    sti
    mov al,byte [fs:0x1a]   ; Process*->curcpu
    mov byte [fs:0x1b],al   ; Process*->lovedcpu
    mov rdi,rbx
    mov rsi,[fs:0x28]   ; Process*->fsbase (self)
    jmp ProcessEntrySafe
