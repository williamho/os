	.globl _start

	.data
hello:
	.ascii "Hello world!\n"

	.text
_start:
	movq	$1, %rax 		# write() syscall number
	movq	$1, %rdi		# stdout
	movq	$hello, %rsi
	movq	$13, %rdx		# length of string
	syscall
	
	movq	$60, %rax		# exit() syscall number
	movq	$0, %rdi
	syscall
	