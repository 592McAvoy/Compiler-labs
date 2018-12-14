.text
.globl tigermain
.type tigermain, @function
tigermain:
subq $0, %rsp
.L5:
leaq 0(%rsp), %rdi
movq $9, %rsi
movq $4, %rdx
call g
movq %rax, %rdi
call printi
jmp .L4
.L4:
addq $0, %rsp
ret

.text
.globl g
.type g, @function
g:
subq $8, %rsp
.L7:
leaq 8(%rsp), %rax
movq %rdi, -8(%rax)
cmpq %rdx, %rsi
jg .L1
.L2:
.L3:
movq %rdx, %rax
jmp .L6
.L1:
movq %rsi, %rdx
jmp .L3
.L6:
addq $8, %rsp
ret

