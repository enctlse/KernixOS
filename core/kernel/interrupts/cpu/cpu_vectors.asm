[BITS 64]

extern exception_processor
extern interrupt_processor

; System Interrupt Management System

; Exception Processing Macro
%macro EXCEPTION_ENTRY 2
global exception_%1
exception_%1:
    %if %2 == 1
        ; Error code present
    %else
        push 0xDEADBEEF    ; Placeholder for no error code
    %endif
    push %1               ; Exception identifier
    jmp common_interrupt_routine
%endmacro

; Define exception entries
EXCEPTION_ENTRY 0, 0    ; Divide error
EXCEPTION_ENTRY 1, 0    ; Debug trap
EXCEPTION_ENTRY 2, 0    ; NMI interrupt
EXCEPTION_ENTRY 3, 0    ; Breakpoint
EXCEPTION_ENTRY 4, 0    ; Overflow
EXCEPTION_ENTRY 5, 0    ; BOUND range exceeded
EXCEPTION_ENTRY 6, 0    ; Invalid opcode
EXCEPTION_ENTRY 7, 0    ; Device not available
EXCEPTION_ENTRY 8, 1    ; Double fault
EXCEPTION_ENTRY 9, 0    ; Coprocessor segment overrun
EXCEPTION_ENTRY 10, 1   ; Invalid TSS
EXCEPTION_ENTRY 11, 1   ; Segment not present
EXCEPTION_ENTRY 12, 1   ; Stack segment fault
EXCEPTION_ENTRY 13, 1   ; General protection
EXCEPTION_ENTRY 14, 1   ; Page fault
EXCEPTION_ENTRY 15, 0   ; Reserved
EXCEPTION_ENTRY 16, 0   ; x87 FPU error
EXCEPTION_ENTRY 17, 1   ; Alignment check
EXCEPTION_ENTRY 18, 0   ; Machine check
EXCEPTION_ENTRY 19, 0   ; SIMD floating-point
EXCEPTION_ENTRY 20, 0   ; Virtualization
EXCEPTION_ENTRY 21, 0   ; Reserved
EXCEPTION_ENTRY 22, 0   ; Reserved
EXCEPTION_ENTRY 23, 0   ; Reserved
EXCEPTION_ENTRY 24, 0   ; Reserved
EXCEPTION_ENTRY 25, 0   ; Reserved
EXCEPTION_ENTRY 26, 0   ; Reserved
EXCEPTION_ENTRY 27, 0   ; Reserved
EXCEPTION_ENTRY 28, 0   ; Reserved
EXCEPTION_ENTRY 29, 0   ; Reserved
EXCEPTION_ENTRY 30, 1   ; Security exception
EXCEPTION_ENTRY 31, 0   ; Reserved

; Common interrupt processing routine
common_interrupt_routine:
    ; Preserve CPU state - save registers in custom order
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Invoke exception handler
    mov rdi, rsp
    call exception_processor

    ; Restore CPU state - restore in reverse custom order
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    ; Remove exception data from stack
    add rsp, 16
    iretq

; Hardware Interrupt Macro
%macro HARDWARE_INTERRUPT 2
global hardware_interrupt_%1
hardware_interrupt_%1:
    push 0xCAFEBABE       ; Dummy value
    push %2               ; Hardware interrupt code
    jmp common_interrupt_routine
%endmacro

; Define hardware interrupts
HARDWARE_INTERRUPT 0, 32
HARDWARE_INTERRUPT 1, 33
HARDWARE_INTERRUPT 2, 34
HARDWARE_INTERRUPT 3, 35
HARDWARE_INTERRUPT 4, 36
HARDWARE_INTERRUPT 5, 37
HARDWARE_INTERRUPT 6, 38
HARDWARE_INTERRUPT 7, 39
HARDWARE_INTERRUPT 8, 40
HARDWARE_INTERRUPT 9, 41
HARDWARE_INTERRUPT 10, 42
HARDWARE_INTERRUPT 11, 43
HARDWARE_INTERRUPT 12, 44
HARDWARE_INTERRUPT 13, 45
HARDWARE_INTERRUPT 14, 46
HARDWARE_INTERRUPT 15, 47

