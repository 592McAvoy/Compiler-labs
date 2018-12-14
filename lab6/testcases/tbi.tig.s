.section .rodata
.L13:
.int 1
.string "\n"
.section .rodata
.L10:
.int 1
.string "\n"
.section .rodata
.L4:
.int 1
.string "y"
.section .rodata
.L3:
.int 1
.string "x"
.text
.globl tigermain
.type tigermain, @function
tigermain:
subq $8, %rsp
.L15:
movq $8, %rax
leaq 8(%rsp), %r11
movq %rax, -8(%r11)
leaq 8(%rsp), %rdi
call printb
jmp .L14
.L14:
addq $8, %rsp
ret

.text
.globl printb
.type printb, @function
printb:
subq $24, %rsp
.L17:
leaq 24(%rsp), %rax
movq %rdi, -8(%rax)
movq $0, %rax
movq %rax, 0(%rsp)
leaq 24(%rsp), %rax
movq -8(%rax), %rax
movq -8(%rax), %rax
movq $1, %r11
subq %r11, %rax
movq 0(%rsp), %r11
cmpq %rax, %r11
jg .L1
.L11:
movq $0, %rax
movq %rax, 8(%rsp)
leaq 24(%rsp), %rax
movq -8(%rax), %rax
movq -8(%rax), %rax
movq $1, %r11
subq %r11, %rax
movq 8(%rsp), %r11
cmpq %rax, %r11
jg .L2
.L8:
movq 0(%rsp), %rax
movq 8(%rsp), %r11
cmpq %r11, %rax
jg .L5
.L6:
leaq .L4, %rdi
.L7:
call print
movq $1, %r11
movq 8(%rsp), %rax
addq %r11, %rax
movq %rax, 8(%rsp)
leaq 24(%rsp), %rax
movq -8(%rax), %rax
movq -8(%rax), %rax
movq $1, %r11
subq %r11, %rax
movq 8(%rsp), %r11
cmpq %rax, %r11
jle .L8
.L2:
leaq .L10, %rdi
call print
movq $1, %r11
movq 0(%rsp), %rax
addq %r11, %rax
movq %rax, 0(%rsp)
leaq 24(%rsp), %rax
movq -8(%rax), %rax
movq -8(%rax), %rax
movq $1, %r11
subq %r11, %rax
movq 0(%rsp), %r11
cmpq %rax, %r11
jle .L11
.L1:
leaq .L13, %rdi
call print
jmp .L16
.L5:
leaq .L3, %rdi
jmp .L7
.L16:
addq $24, %rsp
ret

