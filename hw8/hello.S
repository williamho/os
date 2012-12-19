	.data
hello:
	.string "Hello world!\n"

	.text
	.globl main
main:
	mov	$4, %rax
	mov	$1, %rdi
	mov	$hello, %rsi
	mov	$13, %rdx
	syscall
	mov	$60, %rax
	mov	$1, %rdi
	syscall
	
