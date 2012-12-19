	.text
.globl tas
	.type	tas,@function
tas:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %edx
	movl	$1, %eax
#APP
	lock;xchgb	%al,(%edx)
#NO_APP
	movsbl	%al,%eax
	popl	%ebp
	ret
.Lfe1:
	.size	tas,.Lfe1-tas
