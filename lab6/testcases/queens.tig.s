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
.int 2
.string " ."
.section .rodata
.L3:
.int 2
.string " O"
.text
.globl tigermain
.type tigermain, @function
tigermain:
subq $72, %rsp
.L34:
movq $8, %rax
leaq 72(%rsp), %r11
movq %rax, -8(%r11)
movq $-16, %rax
leaq 72(%rsp), %r11
addq %r11, %rax
movq %rax, 0(%rsp)
leaq 72(%rsp), %rax
movq -8(%rax), %rdi
movq $0, %rsi
call initArray
movq 0(%rsp), %r11
movq %rax, (%r11)
movq $-24, %rax
leaq 72(%rsp), %r11
addq %r11, %rax
movq %rax, 8(%rsp)
leaq 72(%rsp), %rax
movq -8(%rax), %rdi
movq $0, %rsi
call initArray
movq 8(%rsp), %r11
movq %rax, (%r11)
movq $-32, %rax
leaq 72(%rsp), %r11
addq %r11, %rax
movq %rax, 16(%rsp)
leaq 72(%rsp), %rax
movq -8(%rax), %rdi
leaq 72(%rsp), %rax
movq -8(%rax), %rax
addq %rax, %rdi
movq $1, %rax
subq %rax, %rdi
movq $0, %rsi
call initArray
movq 16(%rsp), %r11
movq %rax, (%r11)
movq $-40, %rax
leaq 72(%rsp), %r11
addq %r11, %rax
movq %rax, 24(%rsp)
leaq 72(%rsp), %rax
movq -8(%rax), %rdi
leaq 72(%rsp), %rax
movq -8(%rax), %rax
addq %rax, %rdi
movq $1, %rax
subq %rax, %rdi
movq $0, %rsi
call initArray
movq 24(%rsp), %r11
movq %rax, (%r11)
leaq 72(%rsp), %rdi
movq $0, %rsi
call try
jmp .L33
.L33:
addq $72, %rsp
ret

.text
.globl try
.type try, @function
try:
subq $24, %rsp
.L36:
movq %rsi, 0(%rsp)
leaq 24(%rsp), %rax
movq %rdi, -8(%rax)
leaq 24(%rsp), %rax
movq -8(%rax), %rax
movq -8(%rax), %rax
movq 0(%rsp), %r11
cmpq %rax, %r11
je .L30
.L31:
movq $0, %rax
movq %rax, 8(%rsp)
leaq 24(%rsp), %rax
movq -8(%rax), %rax
movq -8(%rax), %rax
movq $1, %r11
subq %r11, %rax
movq 8(%rsp), %r11
cmpq %rax, %r11
jg .L14
.L28:
movq $8, %rax
movq 8(%rsp), %r11
imulq %r11, %rax
leaq 24(%rsp), %r11
movq -8(%r11), %r11
movq -16(%r11), %r11
addq %r11, %rax
movq (%rax), %rax
movq $0, %r11
cmpq %r11, %rax
je .L15
.L16:
movq $0, %rax
.L17:
movq $0, %r11
cmpq %r11, %rax
jne .L20
.L21:
movq $0, %rax
.L22:
movq $0, %r11
cmpq %r11, %rax
jne .L25
.L26:
.L27:
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
jle .L28
.L14:
.L32:
movq $0, %rax
jmp .L35
.L30:
leaq 24(%rsp), %rax
movq -8(%rax), %rdi
call printboard
jmp .L32
.L15:
movq $1, %rax
movq $8, %r10
movq 8(%rsp), %r11
movq 0(%rsp), %r9
addq %r9, %r11
imulq %r11, %r10
leaq 24(%rsp), %r11
movq -8(%r11), %r11
movq -32(%r11), %r11
addq %r11, %r10
movq (%r10), %r11
movq $0, %r10
cmpq %r10, %r11
je .L18
.L19:
movq $0, %rax
.L18:
jmp .L17
.L20:
movq $1, %rax
movq $8, %r10
movq $7, %r9
movq 8(%rsp), %r11
addq %r9, %r11
movq 0(%rsp), %r9
subq %r9, %r11
imulq %r11, %r10
leaq 24(%rsp), %r11
movq -8(%r11), %r11
movq -40(%r11), %r11
addq %r11, %r10
movq (%r10), %r11
movq $0, %r10
cmpq %r10, %r11
je .L23
.L24:
movq $0, %rax
.L23:
jmp .L22
.L25:
movq $1, %rax
movq $8, %r11
movq 8(%rsp), %r10
imulq %r10, %r11
leaq 24(%rsp), %r10
movq -8(%r10), %r10
movq -16(%r10), %r10
addq %r10, %r11
movq %rax, (%r11)
movq $1, %r11
movq $8, %r10
movq 8(%rsp), %rax
movq 0(%rsp), %r9
addq %r9, %rax
imulq %rax, %r10
leaq 24(%rsp), %rax
movq -8(%rax), %rax
movq -32(%rax), %rax
addq %rax, %r10
movq %r11, (%r10)
movq $1, %r9
movq $8, %r10
movq $7, %r11
movq 8(%rsp), %rax
addq %r11, %rax
movq 0(%rsp), %r11
subq %r11, %rax
imulq %rax, %r10
leaq 24(%rsp), %rax
movq -8(%rax), %rax
movq -40(%rax), %rax
addq %rax, %r10
movq %r9, (%r10)
movq $8, %rax
movq 0(%rsp), %r11
imulq %r11, %rax
leaq 24(%rsp), %r11
movq -8(%r11), %r11
movq -24(%r11), %r11
addq %r11, %rax
movq 8(%rsp), %r11
movq %r11, (%rax)
leaq 24(%rsp), %rax
movq -8(%rax), %rdi
movq $1, %rax
movq 0(%rsp), %rsi
addq %rax, %rsi
call try
movq $0, %rax
movq $8, %r11
movq 8(%rsp), %r10
imulq %r10, %r11
leaq 24(%rsp), %r10
movq -8(%r10), %r10
movq -16(%r10), %r10
addq %r10, %r11
movq %rax, (%r11)
movq $0, %r11
movq $8, %r10
movq 8(%rsp), %rax
movq 0(%rsp), %r9
addq %r9, %rax
imulq %rax, %r10
leaq 24(%rsp), %rax
movq -8(%rax), %rax
movq -32(%rax), %rax
addq %rax, %r10
movq %r11, (%r10)
movq $0, %r11
movq $8, %r10
movq $7, %r9
movq 8(%rsp), %rax
addq %r9, %rax
movq 0(%rsp), %r9
subq %r9, %rax
imulq %rax, %r10
leaq 24(%rsp), %rax
movq -8(%rax), %rax
movq -40(%rax), %rax
addq %rax, %r10
movq %r11, (%r10)
jmp .L27
.L35:
addq $24, %rsp
ret

.text
.globl printboard
.type printboard, @function
printboard:
subq $24, %rsp
.L38:
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
movq $8, %rax
movq 0(%rsp), %r11
imulq %r11, %rax
leaq 24(%rsp), %r11
movq -8(%r11), %r11
movq -24(%r11), %r11
addq %r11, %rax
movq (%rax), %rax
movq 8(%rsp), %r11
cmpq %r11, %rax
je .L5
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
jmp .L37
.L5:
leaq .L3, %rdi
jmp .L7
.L37:
addq $24, %rsp
ret

