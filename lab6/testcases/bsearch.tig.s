.section .rodata
.L11:
.int 1
.string "\n"
.section .rodata
.L1:
.int 0
.string ""
.text
.globl tigermain
.type tigermain, @function
tigermain:
subq $24, %rsp
.L13:
movq $16, %rax
leaq 24(%rsp), %r11
movq %rax, -8(%r11)
movq $-16, %rax
leaq 24(%rsp), %r11
addq %r11, %rax
movq %rax, 0(%rsp)
leaq 24(%rsp), %rax
movq -8(%rax), %rdi
movq $0, %rsi
call initArray
movq 0(%rsp), %r11
movq %rax, (%r11)
leaq 24(%rsp), %rdi
call try
jmp .L12
.L12:
addq $24, %rsp
ret

.text
.globl try
.type try, @function
try:
subq $8, %rsp
.L15:
leaq 8(%rsp), %rax
movq %rdi, -8(%rax)
leaq 8(%rsp), %rax
movq -8(%rax), %rdi
call init
leaq 8(%rsp), %rax
movq -8(%rax), %rdi
movq $0, %rsi
leaq 8(%rsp), %rax
movq -8(%rax), %rax
movq -8(%rax), %rdx
movq $1, %rax
subq %rax, %rdx
movq $7, %rcx
call bsearch
movq %rax, %rdi
call printi
leaq .L11, %rdi
call print
jmp .L14
.L14:
addq $8, %rsp
ret

.text
.globl bsearch
.type bsearch, @function
bsearch:
subq $8, %rsp
.L17:
leaq 8(%rsp), %rax
movq %rdi, -8(%rax)
cmpq %rdx, %rsi
je .L8
.L9:
movq %rsi, %rax
addq %rdx, %rax
movq $2, %r11
idivq %r11
movq $8, %r11
imulq %rax, %r11
leaq 8(%rsp), %r10
movq -8(%r10), %r10
movq -16(%r10), %r10
addq %r10, %r11
movq (%r11), %r11
cmpq %rcx, %r11
jl .L5
.L6:
leaq 8(%rsp), %r11
movq -8(%r11), %rdi
movq %rax, %rdx
call bsearch
.L7:
.L10:
jmp .L16
.L8:
movq %rsi, %rax
jmp .L10
.L5:
leaq 8(%rsp), %r11
movq -8(%r11), %rdi
movq $1, %r11
addq %r11, %rax
movq %rax, %rsi
call bsearch
jmp .L7
.L16:
addq $8, %rsp
ret

.text
.globl init
.type init, @function
init:
subq $16, %rsp
.L19:
leaq 16(%rsp), %rax
movq %rdi, -8(%rax)
movq $0, %rax
movq %rax, 0(%rsp)
leaq 16(%rsp), %rax
movq -8(%rax), %rax
movq -8(%rax), %rax
movq $1, %r11
subq %r11, %rax
movq 0(%rsp), %r11
cmpq %rax, %r11
jg .L2
.L3:
movq $2, %r11
movq 0(%rsp), %rax
imulq %r11, %rax
movq $1, %r11
addq %r11, %rax
movq $8, %r11
movq 0(%rsp), %r10
imulq %r10, %r11
leaq 16(%rsp), %r10
movq -8(%r10), %r10
movq -16(%r10), %r10
addq %r10, %r11
movq %rax, (%r11)
leaq 16(%rsp), %rax
movq -8(%rax), %rdi
call nop
movq $1, %r11
movq 0(%rsp), %rax
addq %r11, %rax
movq %rax, 0(%rsp)
leaq 16(%rsp), %rax
movq -8(%rax), %rax
movq -8(%rax), %rax
movq $1, %r11
subq %r11, %rax
movq 0(%rsp), %r11
cmpq %rax, %r11
jle .L3
.L2:
movq $0, %rax
jmp .L18
.L18:
addq $16, %rsp
ret

.text
.globl nop
.type nop, @function
nop:
subq $8, %rsp
.L21:
leaq 8(%rsp), %rax
movq %rdi, -8(%rax)
leaq .L1, %rdi
call print
jmp .L20
.L20:
addq $8, %rsp
ret

