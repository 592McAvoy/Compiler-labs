.text
.globl tigermain
.type tigermain, @function
tigermain:
subq $0, %rsp
.L2:
leaq 0(%rsp), %rdi
movq $2, %rsi
call a
movq %rax, %rdi
call printi
jmp .L1
.L1:
addq $0, %rsp
ret

.text
.globl a
.type a, @function
a:
subq $16, %rsp
.L4:
leaq 16(%rsp), %rax
movq %rsi, -16(%rax)
leaq 16(%rsp), %rax
movq %rdi, -8(%rax)
leaq 16(%rsp), %rdi
movq $3, %rsi
call b
jmp .L3
.L3:
addq $16, %rsp
ret

.text
.globl b
.type b, @function
b:
subq $8, %rsp
.L6:
leaq 8(%rsp), %rax
movq %rdi, -8(%rax)
leaq 8(%rsp), %rax
movq -8(%rax), %rax
movq -16(%rax), %rax
addq %rsi, %rax
jmp .L5
.L5:
addq $8, %rsp
ret

