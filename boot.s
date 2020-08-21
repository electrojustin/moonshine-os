.set ALIGN, 1<<0
.set MEMINFO, 1<<1
.set FLAGS, ALIGN | MEMINFO
.set MAGIC, 0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)
.set STACK_SIZE, 16384

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss, "aw", @nobits
.align 16
.global stack_bottom
stack_bottom:
.skip STACK_SIZE
.global stack_top
.type stack_top, @common
stack_top:

.align 4096
.global base_page_directory
.type base_page_directory, @common
base_page_directory:
.skip 4096
.global base_page_table
.type base_page_table, @common
base_page_table:
.skip 4194304

.section .text
.global _start
.type _start, @function
_start:
	mov $stack_top, %esp

	push %eax
	push %ebx
	call kernel_main

	cli
loop:	hlt
	jmp loop

.size _start, . - _start
