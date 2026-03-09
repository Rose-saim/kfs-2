bits 32

MBOOT_MAGIC     equ 0x1BADB002
MBOOT_FLAGS     equ 0x00000000
MBOOT_CHECKSUM  equ -(MBOOT_MAGIC + MBOOT_FLAGS)
STACK_SIZE      equ 0x4000

section .multiboot
align 4
    dd MBOOT_MAGIC
    dd MBOOT_FLAGS
    dd MBOOT_CHECKSUM

section .bss
align 16
global stack_bottom
global stack_top
stack_bottom:
    resb STACK_SIZE
stack_top:

section .text
global kernel_start
extern kernel_main

kernel_start:
    mov esp, stack_top
    push ebx
    push eax
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang

global gdt_flush
gdt_flush:
    mov eax, [esp+4]
    lgdt [eax]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush
.flush:
    ret

global tss_flush
tss_flush:
    mov ax, 0x3B
    ltr ax
    ret
