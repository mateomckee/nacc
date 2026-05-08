.section .text
.global fib
fib:
	stp x29, x30, [sp, #-48]!
	mov x29, sp
	str w0, [sp, #0]
fib_end:
	ldp x29, x30, [sp], #48
	ret

.global main
main:
	stp x29, x30, [sp, #-48]!
	mov x29, sp
main_end:
	ldp x29, x30, [sp], #48
	ret

