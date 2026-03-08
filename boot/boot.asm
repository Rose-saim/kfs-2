; boot/boot.asm
; KFS_2 - Multiboot entry point + GDT loading

bits 32

; ─── Multiboot header constants ───────────────────────────────────────────────
MBOOT_MAGIC     equ 0x1BADB002
MBOOT_FLAGS     equ 0x00000003   ; align + memory map
MBOOT_CHECKSUM  equ -(MBOOT_MAGIC + MBOOT_FLAGS)

; ─── GDT must be placed at 0x00000800 ─────────────────────────────────────────
GDT_ADDRESS     equ 0x00000800

; ─── Stack size ───────────────────────────────────────────────────────────────
STACK_SIZE      equ 0x4000       ; 16 KB

section .multiboot
align 4
    dd MBOOT_MAGIC
    dd MBOOT_FLAGS
    dd MBOOT_CHECKSUM

section .bss
align 16
stack_bottom:
    resb STACK_SIZE
global stack_top
stack_top:

section .text
global kernel_start
extern kernel_main

kernel_start:
    ; Set up initial stack
    mov esp, stack_top

    ; Save multiboot info pointer (ebx) and magic (eax) for kernel_main
    push ebx                    ; multiboot info struct pointer
    push eax                    ; multiboot magic number

    ; Call C kernel entry
    call kernel_main

    ; If kernel_main returns, halt forever
.hang:
    cli
    hlt
    jmp .hang

; ─── Load GDT ─────────────────────────────────────────────────────────────────
; Called from C: void gdt_flush(uint32_t gdtr_ptr)
global gdt_flush
gdt_flush:
    mov eax, [esp+4]            ; get pointer to GDT descriptor
    lgdt [eax]                  ; load GDT register

    ; Reload segment registers with kernel data selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to reload CS with kernel code selector (0x08)
    jmp 0x08:.flush
.flush:
    ret

; ─── Load TSS (optional, for completeness) ────────────────────────────────────
global tss_flush
tss_flush:
    mov ax, 0x2B                ; TSS selector (index 5, RPL=3)
    ltr ax
    ret
