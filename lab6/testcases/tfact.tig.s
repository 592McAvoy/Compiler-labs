.text
.globl tigermain
.type tigermain, @function
tigermain:
subq $0, %rsp
.L5:
leaq 0(%rsp), %rdi
movq $10, %rsi
call nfactor
movq %rax, %rdi
call printi
jmp .L4
.L4:
addq $0, %rsp
ret

.text
.globl nfactor
.type nfactor, @function
nfactor:
subq $16, %rsp
.L7:
leaq 16(%rsp), %rax
movq %rdi, -8(%rax)
movq $0, %rax
cmpq %rax, %rsi
je .L1
.L2:
movq %rsi, 0(%rsp)
leaq 16(%rsp), %rax
movq -8(%rax), %rdi
movq $1, %rax
subq %rax, %rsi
call nfactor
movq 0(%rsp), %r11
imulq %rax, %r11
.L3:
movq %r11, %rax
jmp .L6
.L1:
movq $1, %r11
jmp .L3
.L6:
addq $16, %rsp
ret

