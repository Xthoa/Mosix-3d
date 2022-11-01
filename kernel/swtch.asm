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
    mov [rdi],rsp
    mov rsp,[rsi+8]     ; Process* -> rsp
    mov rax,[rsi]    ; Process* -> pagemap
    mov rax,[rax]     ; Pagemap* -> cr3
    mov rbx,cr3
    cmp rbx,rax
    je .sameproc  ; avoid flushing TLB
    mov cr3,rax
    .sameproc:
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
    mov al,byte [fs:26]   ; Process*->curcpu
    mov byte [fs:27],al   ; Process*->lovedcpu
    mov rdi,rbx
    mov rsi,[fs:0x28]
    jmp ProcessEntrySafe