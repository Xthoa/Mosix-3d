bits 64
section .text

global acquire_spin
acquire_spin:   ; rdi safe
    mov al, 0
    .pretest:
    cmp byte [rdi], al
    je .pretest
    xchg byte [rdi], al     ; auto #lock by CPU
    test al, al
    jz .fail
    ret
    .fail:
    pause
    jmp .pretest

global release_spin, init_spinlock
release_spin:
init_spinlock:
    mov byte [rdi],1
    ret

global acquire_mutex
extern block_of_mutex
acquire_mutex:
    pushf
    push rdi
    mov al, 0
    cli
    .try:
    xchg byte [rdi], al     ; auto #lock by CPU
    test al, al
    jz .fail
    pop rdi
    popf
    ret
    .fail:
    call acquire_spin
    call block_of_mutex
    mov rdi, [rsp]
    jmp .try
