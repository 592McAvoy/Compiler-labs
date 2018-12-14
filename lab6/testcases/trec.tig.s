.text
.globl tigermain
.type tigermain, @function
tigermain:
subq $8, %rsp
.L2:
movq $16, %rdi
call malloc
movq $3, %r11
movq %r11, 0(%rax)
movq $4, %r11
movq %r11, 8(%rax)
movq %rax, 0(%rsp)
movq 0(%rsp), %rax
movq 0(%rax), %rdi
call printi
movq 0(%rsp), %rax
movq 8(%rax), %rdi
call printi
jmp .L1
.L1:
addq $8, %rsp
ret

