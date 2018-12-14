.text
.globl tigermain
.type tigermain, @function
tigermain:
subq $8, %rsp
.L8:
movq $10, %rax
movq %rax, 0(%rsp)
.L5:
movq $0, %rax
movq 0(%rsp), %r11
cmpq %rax, %r11
jge .L6
.L1:
movq $0, %rax
jmp .L7
.L6:
movq 0(%rsp), %rdi
call printi
movq $1, %r11
movq 0(%rsp), %rax
subq %r11, %rax
movq %rax, 0(%rsp)
movq $2, %rax
movq 0(%rsp), %r11
cmpq %rax, %r11
je .L2
.L3:
.L4:
jmp .L5
.L2:
jmp .L1
.L9:
jmp .L4
.L7:
addq $8, %rsp
ret

