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
    mov [rdi + 0x14],rsp
    mov rsp,[rsi + 0x14]     ; Process* -> rsp
    mov rax,[rsi + 0x0c]    ; Process* -> vm
    mov rax,[rax]     ; Vmspace* -> cr3
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

global switch_context_exit
switch_context_exit:  ;void switch_context_exit(* now, * next, * now.stat);
    mov rsp,[rsi + 0x14]     ; Process* -> rsp
    mov rax,[rsi + 0x0c]    ; Process* -> vm
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
    mov al,byte [fs:0x26]   ; Process*->curcpu
    mov byte [fs:0x27],al   ; Process*->lovedcpu
    mov rdi,rbx
    mov rsi,[fs:0x34]   ; Process*->fsbase (self)
    jmp ProcessEntrySafe
