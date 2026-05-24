	.file	"main.cpp"
	.text
	.p2align 4
	.type	_ZL8dot_avx2PKfS0_m.constprop.0, @function
_ZL8dot_avx2PKfS0_m.constprop.0:
.LFB7633:
	.cfi_startproc
	vxorps	%xmm2, %xmm2, %xmm2
	leaq	67108864(%rdi), %rax
	vmovaps	%ymm2, %ymm1
	vmovaps	%ymm2, %ymm3
	vmovaps	%ymm2, %ymm0
	.p2align 4,,10
	.p2align 3
.L2:
	vmovaps	(%rdi), %ymm4
	vmovaps	32(%rdi), %ymm5
	subq	$-128, %rdi
	subq	$-128, %rsi
	vmovaps	-64(%rdi), %ymm6
	vmovaps	-32(%rdi), %ymm7
	vfmadd231ps	-128(%rsi), %ymm4, %ymm0
	vfmadd231ps	-96(%rsi), %ymm5, %ymm3
	vfmadd231ps	-64(%rsi), %ymm6, %ymm1
	vfmadd231ps	-32(%rsi), %ymm7, %ymm2
	cmpq	%rdi, %rax
	jne	.L2
	vaddps	%ymm2, %ymm1, %ymm1
	vaddps	%ymm3, %ymm0, %ymm0
	vaddps	%ymm1, %ymm0, %ymm0
	vextractf128	$0x1, %ymm0, %xmm1
	vaddps	%xmm0, %xmm1, %xmm0
	vmovhlps	%xmm0, %xmm0, %xmm1
	vaddps	%xmm0, %xmm1, %xmm1
	vshufps	$85, %xmm1, %xmm1, %xmm0
	vaddps	%xmm1, %xmm0, %xmm0
	vzeroupper
	ret
	.cfi_endproc
.LFE7633:
	.size	_ZL8dot_avx2PKfS0_m.constprop.0, .-_ZL8dot_avx2PKfS0_m.constprop.0
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"alloc failed\n"
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC5:
	.string	"N = %d floats (%.1f MB per array)\n"
	.section	.rodata.str1.1
.LC6:
	.string	"iterations averaged: %d\n\n"
	.section	.rodata.str1.8
	.align 8
.LC9:
	.string	"scalar : %.3f ms   %.2f GB/s   result = %.4f\n"
	.align 8
.LC10:
	.string	"avx2   : %.3f ms   %.2f GB/s   result = %.4f\n"
	.section	.rodata.str1.1
.LC11:
	.string	"speedup: %.2fx\n"
	.section	.rodata.str1.8
	.align 8
.LC13:
	.string	"max abs diff: %.6f (FP reassociation, expected to be small)\n"
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB7482:
	.cfi_startproc
	endbr64
	leaq	8(%rsp), %r10
	.cfi_def_cfa 10, 0
	andq	$-32, %rsp
	movl	$67108864, %edx
	movl	$32, %esi
	pushq	-8(%r10)
	pushq	%rbp
	movq	%rsp, %rbp
	.cfi_escape 0x10,0x6,0x2,0x76,0
	pushq	%r15
	leaq	-96(%rbp), %rdi
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%r10
	.cfi_escape 0xf,0x3,0x76,0x58,0x6
	.cfi_escape 0x10,0xf,0x2,0x76,0x78
	.cfi_escape 0x10,0xe,0x2,0x76,0x70
	.cfi_escape 0x10,0xd,0x2,0x76,0x68
	.cfi_escape 0x10,0xc,0x2,0x76,0x60
	pushq	%rbx
	addq	$-128, %rsp
	.cfi_escape 0x10,0x3,0x2,0x76,0x50
	movq	%fs:40, %rax
	movq	%rax, -56(%rbp)
	xorl	%eax, %eax
	call	posix_memalign@PLT
	testl	%eax, %eax
	jne	.L6
	leaq	-88(%rbp), %rdi
	movl	$67108864, %edx
	movl	$32, %esi
	movq	-96(%rbp), %rbx
	call	posix_memalign@PLT
	movl	%eax, %r13d
	testl	%eax, %eax
	je	.L20
.L6:
	movq	stderr(%rip), %rcx
	movl	$13, %edx
	movl	$1, %esi
	leaq	.LC0(%rip), %rdi
	movl	$1, %r13d
	call	fwrite@PLT
.L5:
	movq	-56(%rbp), %rax
	subq	%fs:40, %rax
	jne	.L21
	subq	$-128, %rsp
	movl	%r13d, %eax
	popq	%rbx
	popq	%r10
	.cfi_remember_state
	.cfi_def_cfa 10, 0
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	leaq	-8(%r10), %rsp
	.cfi_def_cfa 7, 8
	ret
.L20:
	.cfi_restore_state
	movl	$42, %edi
	movq	-88(%rbp), %r12
	xorl	%r14d, %r14d
	call	srand@PLT
	.p2align 4,,10
	.p2align 3
.L7:
	call	rand@PLT
	vxorps	%xmm4, %xmm4, %xmm4
	movslq	%eax, %rdx
	movl	%eax, %ecx
	imulq	$274877907, %rdx, %rdx
	sarl	$31, %ecx
	sarq	$38, %rdx
	subl	%ecx, %edx
	imull	$1000, %edx, %edx
	subl	%edx, %eax
	vcvtsi2ssl	%eax, %xmm4, %xmm0
	vmulss	.LC1(%rip), %xmm0, %xmm0
	vmovss	%xmm0, (%rbx,%r14,4)
	call	rand@PLT
	vxorps	%xmm4, %xmm4, %xmm4
	movslq	%eax, %rdx
	movl	%eax, %ecx
	imulq	$274877907, %rdx, %rdx
	sarl	$31, %ecx
	sarq	$38, %rdx
	subl	%ecx, %edx
	imull	$1000, %edx, %edx
	subl	%edx, %eax
	vcvtsi2ssl	%eax, %xmm4, %xmm0
	vmulss	.LC1(%rip), %xmm0, %xmm0
	vmovss	%xmm0, (%r12,%r14,4)
	incq	%r14
	cmpq	$16777216, %r14
	jne	.L7
	xorl	%eax, %eax
	vxorps	%xmm0, %xmm0, %xmm0
.L9:
	vmovaps	(%r12,%rax), %ymm3
	vmovaps	32(%r12,%rax), %ymm2
	vfmadd231ps	(%rbx,%rax), %ymm3, %ymm0
	vfmadd231ps	32(%rbx,%rax), %ymm2, %ymm0
	addq	$64, %rax
	cmpq	$67108864, %rax
	jne	.L9
	vextractf128	$0x1, %ymm0, %xmm1
	movq	%r12, %rsi
	leaq	-80(%rbp), %r14
	movq	%rbx, %rdi
	vaddps	%xmm0, %xmm1, %xmm0
	vmovhlps	%xmm0, %xmm0, %xmm1
	vaddps	%xmm0, %xmm1, %xmm1
	vshufps	$85, %xmm1, %xmm1, %xmm0
	vaddps	%xmm1, %xmm0, %xmm0
	vmovd	%xmm0, %edx
	vzeroupper
	call	_ZL8dot_avx2PKfS0_m.constprop.0
	vmovd	%edx, %xmm2
	movq	%r14, %rsi
	movl	$1, %edi
	vaddss	%xmm0, %xmm2, %xmm0
	vmovss	%xmm0, -100(%rbp)
	vmovss	-100(%rbp), %xmm0
	call	clock_gettime@PLT
	vxorpd	%xmm3, %xmm3, %xmm3
	movl	$20, %edx
	vcvtsi2sdq	-80(%rbp), %xmm3, %xmm0
	vmovsd	%xmm0, -120(%rbp)
	vcvtsi2sdq	-72(%rbp), %xmm3, %xmm0
	vmulsd	.LC2(%rip), %xmm0, %xmm3
	vmovsd	%xmm3, -128(%rbp)
	.p2align 4,,10
	.p2align 3
.L10:
	xorl	%eax, %eax
	vxorps	%xmm0, %xmm0, %xmm0
.L11:
	vmovaps	(%r12,%rax), %ymm5
	vmovaps	32(%r12,%rax), %ymm6
	vfmadd231ps	(%rbx,%rax), %ymm5, %ymm0
	vfmadd231ps	32(%rbx,%rax), %ymm6, %ymm0
	addq	$64, %rax
	cmpq	$67108864, %rax
	jne	.L11
	decl	%edx
	jne	.L10
	vextractf128	$0x1, %ymm0, %xmm1
	movq	%r14, %rsi
	movl	$1, %edi
	vaddps	%xmm0, %xmm1, %xmm1
	vmovhlps	%xmm1, %xmm1, %xmm0
	vaddps	%xmm1, %xmm0, %xmm0
	vshufps	$85, %xmm0, %xmm0, %xmm1
	vaddps	%xmm0, %xmm1, %xmm0
	vmovd	%xmm0, %r15d
	vzeroupper
	call	clock_gettime@PLT
	vxorpd	%xmm7, %xmm7, %xmm7
	vmovsd	.LC2(%rip), %xmm2
	movq	%r14, %rsi
	vcvtsi2sdq	-72(%rbp), %xmm7, %xmm1
	movl	$1, %edi
	vcvtsi2sdq	-80(%rbp), %xmm7, %xmm0
	vfmsub213sd	-120(%rbp), %xmm1, %xmm2
	vsubsd	-128(%rbp), %xmm0, %xmm1
	vaddsd	%xmm2, %xmm1, %xmm1
	vmulsd	.LC3(%rip), %xmm1, %xmm2
	vmovsd	%xmm1, -152(%rbp)
	vmovsd	%xmm2, -120(%rbp)
	call	clock_gettime@PLT
	vxorpd	%xmm7, %xmm7, %xmm7
	movq	%r12, %rsi
	movq	%rbx, %rdi
	vcvtsi2sdq	-80(%rbp), %xmm7, %xmm0
	vmovsd	%xmm0, -136(%rbp)
	vcvtsi2sdq	-72(%rbp), %xmm7, %xmm0
	vmovsd	%xmm0, -144(%rbp)
	call	_ZL8dot_avx2PKfS0_m.constprop.0
	movq	%r14, %rsi
	movl	$1, %edi
	vmovss	%xmm0, -128(%rbp)
	call	clock_gettime@PLT
	vxorpd	%xmm7, %xmm7, %xmm7
	vmovsd	.LC2(%rip), %xmm1
	vmovsd	.LC2(%rip), %xmm3
	vcvtsi2sdq	-80(%rbp), %xmm7, %xmm0
	movl	$16777216, %edx
	leaq	.LC5(%rip), %rsi
	movl	$2, %edi
	movl	$1, %eax
	vmovsd	%xmm0, %xmm0, %xmm2
	vcvtsi2sdq	-72(%rbp), %xmm7, %xmm0
	vfnmadd231sd	-144(%rbp), %xmm1, %xmm2
	vfmsub213sd	-136(%rbp), %xmm0, %xmm3
	vmovsd	.LC4(%rip), %xmm0
	vaddsd	%xmm2, %xmm3, %xmm3
	vmulsd	.LC3(%rip), %xmm3, %xmm1
	vmovsd	%xmm3, -136(%rbp)
	vmovq	%xmm1, %r14
	call	__printf_chk@PLT
	movl	$20, %edx
	leaq	.LC6(%rip), %rsi
	movl	$2, %edi
	xorl	%eax, %eax
	call	__printf_chk@PLT
	vmovsd	.LC7(%rip), %xmm7
	vmovsd	-152(%rbp), %xmm1
	leaq	.LC9(%rip), %rsi
	vmovsd	.LC8(%rip), %xmm3
	movl	$2, %edi
	movl	$3, %eax
	vdivsd	%xmm1, %xmm7, %xmm1
	vmulsd	-120(%rbp), %xmm3, %xmm0
	vmovd	%r15d, %xmm3
	vcvtss2sd	%xmm3, %xmm3, %xmm2
	call	__printf_chk@PLT
	vmovq	%r14, %xmm3
	vmovsd	.LC7(%rip), %xmm1
	leaq	.LC10(%rip), %rsi
	vmulsd	.LC8(%rip), %xmm3, %xmm0
	vmovsd	-136(%rbp), %xmm3
	movl	$2, %edi
	movl	$3, %eax
	vcvtss2sd	-128(%rbp), %xmm2, %xmm2
	vdivsd	%xmm3, %xmm1, %xmm1
	call	__printf_chk@PLT
	vmovsd	-120(%rbp), %xmm2
	vmovq	%r14, %xmm7
	leaq	.LC11(%rip), %rsi
	movl	$2, %edi
	movl	$1, %eax
	vdivsd	%xmm7, %xmm2, %xmm0
	call	__printf_chk@PLT
	vmovd	%r15d, %xmm2
	leaq	.LC13(%rip), %rsi
	movl	$2, %edi
	vsubss	-128(%rbp), %xmm2, %xmm0
	movl	$1, %eax
	vandps	.LC12(%rip), %xmm0, %xmm0
	vcvtss2sd	%xmm0, %xmm0, %xmm0
	call	__printf_chk@PLT
	movq	%rbx, %rdi
	call	free@PLT
	movq	%r12, %rdi
	call	free@PLT
	jmp	.L5
.L21:
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE7482:
	.size	main, .-main
	.section	.rodata.cst4,"aM",@progbits,4
	.align 4
.LC1:
	.long	981668463
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC2:
	.long	-400107883
	.long	1041313291
	.align 8
.LC3:
	.long	-1717986918
	.long	1068079513
	.align 8
.LC4:
	.long	0
	.long	1078984704
	.align 8
.LC7:
	.long	0
	.long	1074003968
	.align 8
.LC8:
	.long	0
	.long	1083129856
	.section	.rodata.cst16,"aM",@progbits,16
	.align 16
.LC12:
	.long	2147483647
	.long	0
	.long	0
	.long	0
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
