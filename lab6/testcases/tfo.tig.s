.text
.globl tigermain
.type tigermain, @function
tigermain:
subq $16, %rsp
.L8:
movq $4, %rax
movq %rax, 0(%rsp)
movq $0, %rax
movq %rax, 8(%rsp)
movq 8(%rsp), %rax
movq 0(%rsp), %r11
cmpq %r11, %rax
jg .L1
.L5:
movq 8(%rsp), %rdi
call printi
movq $3, %rax
movq 8(%rsp), %r11
cmpq %rax, %r11
je .L2
.L3:
.L4:
movq $1, %r11
movq 8(%rsp), %rax
addq %r11, %rax
movq %rax, 8(%rsp)
movq 8(%rsp), %rax
movq 0(%rsp), %r11
cmpq %r11, %rax
jle .L5
.L1:
movq $0, %rax
jmp .L7
.L2:
jmp .L1
.L9:
jmp .L4
.L7:
addq $16, %rsp
ret

