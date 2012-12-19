#include "jmpbuf-offsets64.h"

	.text
	.global	savectx
	.global restorectx
/* This looks like a function to C, but we skip the usual stack */
/* frame stuff since there are no local variables.  Only need to save */
/* the callee-save registers.  First arg (jb addr) is in rdi */
savectx:

	movq	%rbx, (JB_BX*8)(%rdi)
	movq	%rbp, (JB_BP*8)(%rdi)
	movq	%r12, (JB_12*8)(%rdi)
	movq	%r13, (JB_13*8)(%rdi)
	movq	%r14, (JB_14*8)(%rdi)
	movq	%r15, (JB_15*8)(%rdi)
	leaq	8(%rsp), %rdx			/*SP when we return */
	movq	%rdx, (JB_SP*8)(%rdi)
	movq	0(%rsp), %rax			/*return address */
	movq	%rax, (JB_PC*8)(%rdi)		
	xorl	%eax,%eax			/*return val 0 (int 32)*/
	retq

/* Call restorectx(void *jmpbuf, int arg)		*/
/* jmpbuf will be in rdi, arg will be in esi		*/
restorectx:
	movq	(JB_PC*8)(%rdi),%rdx	/*Save return addr in edx */
	/* Restore all registers */
	movq	(JB_BX*8)(%rdi),%rbx
	movq	(JB_12*8)(%rdi),%r12
	movq	(JB_13*8)(%rdi),%r13
	movq	(JB_14*8)(%rdi),%r14
	movq	(JB_15*8)(%rdi),%r15
	movq	(JB_BX*8)(%rdi),%rbx
	movq	(JB_BP*8)(%rdi),%rbp
	movq	(JB_SP*8)(%rdi),%rsp
	movl	%esi, %eax		/* Ret val is 32-bit int */
	/* Jump to setjmp point */
	jmpq	*%rdx
