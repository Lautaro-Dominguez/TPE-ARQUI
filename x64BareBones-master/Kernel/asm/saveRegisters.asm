section .bss
registers resb 144    ; 18 registros × 8 bytes = 144 bytes

section .text
global saveRegisters
global getRegisters

saveRegisters:
    mov [registers + 0x00], rax
    mov [registers + 0x08], rbx
    mov [registers + 0x10], rcx
    mov [registers + 0x18], rdx
    mov [registers + 0x20], rsi
    mov [registers + 0x28], rdi
    mov [registers + 0x30], rsp
    mov [registers + 0x38], rbp
    mov [registers + 0x40], r8
    mov [registers + 0x48], r9
    mov [registers + 0x50], r10
    mov [registers + 0x58], r11
    mov [registers + 0x60], r12
    mov [registers + 0x68], r13
    mov [registers + 0x70], r14
    mov [registers + 0x78], r15
    ; Guardar registro RIP 
    mov rax, [rsp]  ;direccion de retorno apuntada por rsp 
    mov [registers + 0x80], rax
    ; Guardar RFLAGS
    pushfq
    pop rax
    mov [registers + 0x88], rax
    ret

getRegisters:
    mov rax, registers
    ret