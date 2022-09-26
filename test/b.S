	.file	"b.c"
	.text
	.def	_atomic_inc_and_setg;	.scl	3;	.type	32;	.endef
_atomic_inc_and_setg:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$16, %esp
	movl	8(%ebp), %eax
	movl	8(%ebp), %edx
/APP
 # 34 "b.c" 1
	incl (%eax); setg -1(%ebp)
 # 0 "" 2
/NO_APP
	movzbl	-1(%ebp), %eax
	movzbl	%al, %eax
	leave
	ret
	.def	_atomic_dec_and_setge;	.scl	3;	.type	32;	.endef
_atomic_dec_and_setge:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$16, %esp
	movl	$0, -4(%ebp)
	movl	8(%ebp), %eax
	movl	8(%ebp), %edx
/APP
 # 43 "b.c" 1
	decl (%eax); setge -4(%ebp)
 # 0 "" 2
/NO_APP
	movl	-4(%ebp), %eax
	leave
	ret
	.comm	_sem, 4, 2
	.def	___main;	.scl	2;	.type	32;	.endef
	.section .rdata,"dr"
LC0:
	.ascii "%d %d\12\0"
	.text
	.globl	_main
	.def	_main;	.scl	2;	.type	32;	.endef
_main:
	pushl	%ebp
	movl	%esp, %ebp
	andl	$-16, %esp
	subl	$32, %esp
	call	___main
	movl	$100, 28(%esp)
	movl	$456, 24(%esp)
	movl	28(%esp), %eax
/APP
 # 59 "b.c" 1
	xchg %eax, 24(%esp)
 # 0 "" 2
/NO_APP
	movl	%eax, 28(%esp)
	movl	24(%esp), %eax
	movl	%eax, 8(%esp)
	movl	28(%esp), %eax
	movl	%eax, 4(%esp)
	movl	$LC0, (%esp)
	call	_printf
	movl	$0, %eax
	leave
	ret
	.globl	_sema_init
	.def	_sema_init;	.scl	2;	.type	32;	.endef
_sema_init:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax
	movl	12(%ebp), %edx
	movl	%edx, (%eax)
	nop
	popl	%ebp
	ret
	.section .rdata,"dr"
LC1:
	.ascii "sema_count:%d\12\0"
LC2:
	.ascii "sema_wake!!\0"
LC3:
	.ascii "sema_post!!\0"
	.text
	.globl	_sema_post
	.def	_sema_post;	.scl	2;	.type	32;	.endef
_sema_post:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	8(%ebp), %eax
	movl	(%eax), %eax
	movl	%eax, 4(%esp)
	movl	$LC1, (%esp)
	call	_printf
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_atomic_inc_and_setg
	testl	%eax, %eax
	jne	L9
	movl	$LC2, (%esp)
	call	_puts
	jmp	L11
L9:
	movl	$LC3, (%esp)
	call	_puts
L11:
	nop
	leave
	ret
	.section .rdata,"dr"
LC4:
	.ascii "sema_wait!!\0"
LC5:
	.ascii "sema_down!!\0"
	.text
	.globl	_sema_wait
	.def	_sema_wait;	.scl	2;	.type	32;	.endef
_sema_wait:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	8(%ebp), %eax
	movl	(%eax), %eax
	movl	%eax, 4(%esp)
	movl	$LC1, (%esp)
	call	_printf
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_atomic_dec_and_setge
	testl	%eax, %eax
	jne	L13
	movl	$LC4, (%esp)
	call	_puts
	jmp	L15
L13:
	movl	$LC5, (%esp)
	call	_puts
L15:
	nop
	leave
	ret
	.ident	"GCC: (x86_64-posix-sjlj-rev0, Built by MinGW-W64 project) 8.1.0"
	.def	_printf;	.scl	2;	.type	32;	.endef
	.def	_puts;	.scl	2;	.type	32;	.endef
