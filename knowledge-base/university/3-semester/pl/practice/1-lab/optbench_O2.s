	.file	"OPTBENCH.c"
	.intel_syntax noprefix
 # GNU C23 (x86_64-posix-seh-rev0, Built by MinGW-Builds project) version 15.2.0 (x86_64-w64-mingw32)
 #	compiled by GNU C version 15.2.0, GMP version 6.2.1, MPFR version 4.1.0, MPC version 1.2.1, isl version isl-0.27-GMP

 # GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
 # options passed: -masm=intel -mtune=core2 -march=nocona -O2
	.text
	.p2align 4
	.globl	dead_code
	.def	dead_code;	.scl	2;	.type	32;	.endef
	.seh_proc	dead_code
dead_code:
	.seh_endprologue
 # OPTBENCH.c:220:             } /* Конец dead_code */
	ret	
	.seh_endproc
	.p2align 4
	.globl	unnecessary_loop
	.def	unnecessary_loop;	.scl	2;	.type	32;	.endef
	.seh_proc	unnecessary_loop
unnecessary_loop:
	.seh_endprologue
	mov	eax, DWORD PTR j5[rip]	 # j5, j5
	mov	DWORD PTR i[rip], 5	 # i,
	mov	DWORD PTR k5[rip], eax	 # k5, j5
 # OPTBENCH.c:238:             } /* Конец unnecessary_loop */
	ret	
	.seh_endproc
	.p2align 4
	.globl	loop_jamming
	.def	loop_jamming;	.scl	2;	.type	32;	.endef
	.seh_proc	loop_jamming
loop_jamming:
	.seh_endprologue
 # OPTBENCH.c:251:                     k5 = x + j5 * i;
	mov	eax, DWORD PTR j5[rip]	 # j5, j5
	mov	DWORD PTR i[rip], 5	 # i,
	lea	eax, [rcx+rax*4]	 # _3,
	mov	DWORD PTR k5[rip], eax	 # k5, _3
 # OPTBENCH.c:253:                     i5 = x * k5 * i;
	imul	eax, ecx	 # _7, x
 # OPTBENCH.c:253:                     i5 = x * k5 * i;
	sal	eax, 2	 # tmp107,
	mov	DWORD PTR i5[rip], eax	 # i5, tmp107
 # OPTBENCH.c:254:             } /* Конец loop_jamming */
	ret	
	.seh_endproc
	.section .rdata,"dr"
.LC1:
	.ascii "Hello\0"
	.align 8
.LC5:
	.ascii "Common subexpression elimination\0"
	.section	.text.startup,"x"
	.p2align 4
	.globl	main
	.def	main;	.scl	2;	.type	32;	.endef
	.seh_proc	main
main:
	push	rbx	 #
	.seh_pushreg	rbx
	sub	rsp, 32	 #,
	.seh_stackalloc	32
	.seh_endprologue
 # OPTBENCH.c:53:          {
	call	__main	 #
 # OPTBENCH.c:57: 			 for(i = 0; i < 3 ; i++) ivector[ i ] = 1;
	mov	rax, QWORD PTR .LC0[rip]	 # tmp123,
	mov	DWORD PTR ivector[rip+8], 1	 # ivector[2],
	mov	DWORD PTR i[rip], 3	 # i,
 # OPTBENCH.c:64: 			 i2 = j4;
	mov	DWORD PTR i2[rip], 6	 # i2,
 # OPTBENCH.c:57: 			 for(i = 0; i < 3 ; i++) ivector[ i ] = 1;
	mov	QWORD PTR ivector[rip], rax	 # MEM <vector(2) int> [(int *)&ivector], tmp123
 # OPTBENCH.c:75:             j4 = k5;
	mov	eax, DWORD PTR k5[rip]	 # k5.8_3, k5
 # OPTBENCH.c:76:             if( i2 < j4 && i4 < j4 ){
	cmp	eax, 6	 # k5.8_3,
 # OPTBENCH.c:75:             j4 = k5;
	mov	DWORD PTR j4[rip], eax	 # j4, k5.8_3
 # OPTBENCH.c:76:             if( i2 < j4 && i4 < j4 ){
	jle	.L6	 #,
 # OPTBENCH.c:76:             if( i2 < j4 && i4 < j4 ){
	cmp	eax, DWORD PTR i4[rip]	 # k5.8_3, i4
	jg	.L16	 #,
.L6:
 # OPTBENCH.c:87:             flt_1 = 2.4 + 6.3;
	mov	rax, QWORD PTR .LC2[rip]	 # tmp157,
 # OPTBENCH.c:86:             i3 = 1 + 2;
	mov	DWORD PTR i3[rip], 3	 # i3,
 # OPTBENCH.c:88:             i2 = 5;
	mov	DWORD PTR i2[rip], 5	 # i2,
 # OPTBENCH.c:114:             k3 = 1;
	mov	DWORD PTR k3[rip], 1	 # k3,
 # OPTBENCH.c:123: 		ivector4[ i ] = i * 2;
	mov	DWORD PTR ivector4[rip+8], 655368	 # MEM <vector(2) short int> [(short int *)&ivector4 + 8B],
 # OPTBENCH.c:87:             flt_1 = 2.4 + 6.3;
	mov	QWORD PTR flt_1[rip], rax	 # flt_1, tmp157
 # OPTBENCH.c:89:             j2 = i + 0;
	mov	eax, DWORD PTR i[rip]	 # i.13_5, i
	mov	DWORD PTR i5[rip], 0	 # i5,
	mov	DWORD PTR j2[rip], eax	 # j2, i.13_5
 # OPTBENCH.c:91:             i4 = i * 1;
	mov	DWORD PTR i4[rip], eax	 # i4, i.13_5
 # OPTBENCH.c:121: 	    k2 = 4 * j5;
	mov	eax, DWORD PTR j5[rip]	 # tmp158, j5
	mov	DWORD PTR j5[rip], 10000	 # j5,
	sal	eax, 2	 # _7,
 # OPTBENCH.c:121: 	    k2 = 4 * j5;
	mov	DWORD PTR k2[rip], eax	 # k2, _7
 # OPTBENCH.c:123: 		ivector4[ i ] = i * 2;
	mov	rax, QWORD PTR .LC3[rip]	 # tmp132,
	mov	QWORD PTR ivector4[rip], rax	 # MEM <vector(4) short int> [(short int *)&ivector4], tmp132
	lea	rax, ivector5[rip+12]	 # ivtmp.121,
	lea	rdx, 800[rax]	 # _18,
	.p2align 5
	.p2align 4,,10
	.p2align 3
.L7:
 # OPTBENCH.c:141:                 ivector5[ i * 2 + 3 ] = 5;
	mov	DWORD PTR [rax], 5	 # MEM[(int *)_14],
 # OPTBENCH.c:140:             for( i = 0; i < 100; i++ )
	add	rax, 16	 # ivtmp.121,
 # OPTBENCH.c:141:                 ivector5[ i * 2 + 3 ] = 5;
	mov	DWORD PTR -8[rax], 5	 # MEM[(int *)_14],
 # OPTBENCH.c:140:             for( i = 0; i < 100; i++ )
	cmp	rdx, rax	 # _18, ivtmp.121
	jne	.L7	 #,
	mov	DWORD PTR i[rip], 100	 # i,
 # OPTBENCH.c:168:             if(( h3 + k3 ) < 0 || ( h3 + k3 ) > 5 )
	mov	ecx, DWORD PTR h3[rip]	 # h3.36_25, h3
 # OPTBENCH.c:150:                 k5 = i5 + i2;
	mov	DWORD PTR k5[rip], 5	 # k5,
 # OPTBENCH.c:158:             ivector[ 0 ] = 1;  /* генерация константного адреса */
	mov	DWORD PTR ivector[rip], 1	 # ivector[0],
 # OPTBENCH.c:159:             ivector[ i2 ] = 2; /* значение i2 должно быть скопировано*/
	mov	DWORD PTR ivector[rip+20], 2	 # ivector[5],
 # OPTBENCH.c:161:             ivector[ 2 ] = 3;  /* генарация константного адреса */
	mov	DWORD PTR ivector[rip+8], 3	 # ivector[2],
 # OPTBENCH.c:168:             if(( h3 + k3 ) < 0 || ( h3 + k3 ) > 5 )
	lea	edx, 1[rcx]	 # _26,
 # OPTBENCH.c:168:             if(( h3 + k3 ) < 0 || ( h3 + k3 ) > 5 )
	cmp	edx, 5	 # _26,
	ja	.L17	 #,
 # OPTBENCH.c:171:                 m3 = ( h3 + k3 ) / i3;
	movsx	rax, edx	 # _26, _26
	sar	edx, 31	 # tmp142,
 # OPTBENCH.c:172: 		g3 = i3 + (h3 + k3);
	add	ecx, 4	 # tmp144,
 # OPTBENCH.c:171:                 m3 = ( h3 + k3 ) / i3;
	imul	rax, rax, 1431655766	 # tmp140, _26,
 # OPTBENCH.c:172: 		g3 = i3 + (h3 + k3);
	mov	DWORD PTR g3[rip], ecx	 # g3, tmp144
 # OPTBENCH.c:171:                 m3 = ( h3 + k3 ) / i3;
	shr	rax, 32	 # tmp141,
	sub	eax, edx	 # tmp143, tmp142
	mov	DWORD PTR m3[rip], eax	 # m3, tmp143
.L9:
 # OPTBENCH.c:180:             for( i4 = 0; i4 <= max_vector; i4++){
	mov	DWORD PTR i4[rip], 0	 # i4,
	lea	rbx, ivector2[rip]	 # tmp154,
	.p2align 4,,10
	.p2align 3
.L10:
 # OPTBENCH.c:181: 				printf("Hello");
	lea	rcx, .LC1[rip]	 #,
	call	printf	 #
 # OPTBENCH.c:182:                 ivector2[ i4 ] = j * k;
	movsx	r8, DWORD PTR i4[rip]	 #, i4
 # OPTBENCH.c:182:                 ivector2[ i4 ] = j * k;
	movzx	eax, BYTE PTR k[rip]	 # _35, k
	mul	BYTE PTR j[rip]	 # j
 # OPTBENCH.c:182:                 ivector2[ i4 ] = j * k;
	mov	rdx, r8	 #,
 # OPTBENCH.c:182:                 ivector2[ i4 ] = j * k;
	mov	BYTE PTR [rbx+r8], al	 # ivector2[i4.46_34], _35
 # OPTBENCH.c:180:             for( i4 = 0; i4 <= max_vector; i4++){
	add	edx, 1	 # _36,
 # OPTBENCH.c:180:             for( i4 = 0; i4 <= max_vector; i4++){
	cmp	edx, 2	 # _36,
 # OPTBENCH.c:180:             for( i4 = 0; i4 <= max_vector; i4++){
	mov	DWORD PTR i4[rip], edx	 # i4, _36
 # OPTBENCH.c:180:             for( i4 = 0; i4 <= max_vector; i4++){
	jle	.L10	 #,
 # OPTBENCH.c:195:             unnecessary_loop();
	call	unnecessary_loop	 #
 # OPTBENCH.c:198: 	loop_jamming(7);
	mov	ecx, 7	 #,
	call	loop_jamming	 #
 # OPTBENCH.c:202:          }    /* Конец функции main */
	xor	eax, eax	 #
 # OPTBENCH.c:269:                     ivector4[ i ] = 0;
	mov	QWORD PTR ivector4[rip], 0	 # MEM <char[1:12]> [(void *)&ivector4],
	mov	DWORD PTR ivector4[rip+8], 0	 # MEM <char[1:12]> [(void *)&ivector4],
	mov	DWORD PTR i[rip], 6	 # i,
 # OPTBENCH.c:202:          }    /* Конец функции main */
	add	rsp, 32	 #,
	pop	rbx	 #
	ret	
.L17:
 # OPTBENCH.c:169:                 printf("Common subexpression elimination\n");
	lea	rcx, .LC5[rip]	 #,
	call	puts	 #
	jmp	.L9	 #
.L16:
 # OPTBENCH.c:78: 				printf("Hello");
	lea	rcx, .LC1[rip]	 #,
 # OPTBENCH.c:77:                 i5 = 3;
	mov	DWORD PTR i5[rip], 3	 # i5,
 # OPTBENCH.c:78: 				printf("Hello");
	call	printf	 #
	jmp	.L6	 #
	.seh_endproc
	.text
	.p2align 4
	.globl	loop_unrolling
	.def	loop_unrolling;	.scl	2;	.type	32;	.endef
	.seh_proc	loop_unrolling
loop_unrolling:
	.seh_endprologue
 # OPTBENCH.c:269:                     ivector4[ i ] = 0;
	mov	QWORD PTR ivector4[rip], 0	 # MEM <char[1:12]> [(void *)&ivector4],
	mov	DWORD PTR ivector4[rip+8], 0	 # MEM <char[1:12]> [(void *)&ivector4],
	mov	DWORD PTR i[rip], 6	 # i,
 # OPTBENCH.c:270: 	    } /* Конец loop_unrolling */
	ret	
	.seh_endproc
	.p2align 4
	.globl	jump_compression
	.def	jump_compression;	.scl	2;	.type	32;	.endef
	.seh_proc	jump_compression
jump_compression:
	.seh_endprologue
 # OPTBENCH.c:281:            {
	mov	r10d, DWORD PTR 40[rsp]	 # m, m
 # OPTBENCH.c:283:                if( i < j )
	cmp	edx, ecx	 # j, i
	jg	.L20	 #,
	jmp	.L21	 #
	.p2align 4,,10
	.p2align 3
.L28:
 # OPTBENCH.c:285:                        if( k < l )
	cmp	r8d, r9d	 # k, l
	jge	.L23	 #,
 # OPTBENCH.c:286: 			   if( l < m )
	cmp	r9d, r10d	 # l, m
	jl	.L27	 #,
 # OPTBENCH.c:283:                if( i < j )
	cmp	edx, ecx	 # j, i
	jle	.L21	 #,
.L20:
 # OPTBENCH.c:284:                    if( j < k )
	cmp	r8d, edx	 # k, j
	jg	.L28	 #,
 # OPTBENCH.c:293:                        j += k;
	add	edx, r8d	 # j, k
 # OPTBENCH.c:283:                if( i < j )
	cmp	edx, ecx	 # j, i
	jg	.L20	 #,
.L21:
 # OPTBENCH.c:298:                    i += j;
	add	ecx, edx	 # i, j
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	lea	eax, [rcx+rdx]	 # _1,
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	add	eax, r8d	 # _2, k
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	add	eax, r9d	 # _3, l
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	add	eax, r10d	 # _18, m
 # OPTBENCH.c:300:            } /* Конец jump_compression */
	ret	
	.p2align 4,,10
	.p2align 3
.L23:
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	lea	eax, [rcx+rdx]	 # _1,
 # OPTBENCH.c:291:                            k += l;
	add	r8d, r9d	 # k, l
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	add	eax, r8d	 # _2, k
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	add	eax, r9d	 # _3, l
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	add	eax, r10d	 # _18, m
 # OPTBENCH.c:300:            } /* Конец jump_compression */
	ret	
	.p2align 4,,10
	.p2align 3
.L27:
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	lea	eax, [rcx+rdx]	 # _1,
 # OPTBENCH.c:287:                                l += m;
	add	r9d, r10d	 # l, m
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	add	eax, r8d	 # _2, k
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	add	eax, r9d	 # _3, l
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	add	eax, r10d	 # _18, m
 # OPTBENCH.c:300:            } /* Конец jump_compression */
	ret	
	.seh_endproc
	.globl	ivector5
	.bss
	.align 32
ivector5:
	.space 400
	.globl	ivector4
	.align 16
ivector4:
	.space 12
	.globl	ivector2
ivector2:
	.space 3
	.globl	ivector
	.align 16
ivector:
	.space 12
	.globl	flt_6
	.align 8
flt_6:
	.space 8
	.globl	flt_5
	.align 8
flt_5:
	.space 8
	.globl	flt_4
	.align 8
flt_4:
	.space 8
	.globl	flt_3
	.align 8
flt_3:
	.space 8
	.globl	flt_2
	.align 8
flt_2:
	.space 8
	.globl	flt_1
	.align 8
flt_1:
	.space 8
	.globl	k5
	.align 4
k5:
	.space 4
	.globl	j5
	.align 4
j5:
	.space 4
	.globl	i5
	.align 4
i5:
	.space 4
	.globl	j4
	.align 4
j4:
	.space 4
	.globl	i4
	.align 4
i4:
	.space 4
	.globl	m3
	.align 4
m3:
	.space 4
	.globl	k3
	.align 4
k3:
	.space 4
	.globl	i3
	.align 4
i3:
	.space 4
	.globl	h3
	.align 4
h3:
	.space 4
	.globl	g3
	.align 4
g3:
	.space 4
	.globl	k2
	.align 4
k2:
	.space 4
	.globl	j2
	.align 4
j2:
	.space 4
	.globl	i2
	.align 4
i2:
	.space 4
	.globl	m
	.align 4
m:
	.space 4
	.globl	l
	.align 4
l:
	.space 4
	.globl	k
	.align 4
k:
	.space 4
	.globl	j
	.align 4
j:
	.space 4
	.globl	i
	.align 4
i:
	.space 4
	.section .rdata,"dr"
	.align 8
.LC0:
	.long	1
	.long	1
	.align 8
.LC2:
	.long	1717986918
	.long	1075930726
	.align 8
.LC3:
	.word	0
	.word	2
	.word	4
	.word	6
	.def	__main;	.scl	2;	.type	32;	.endef
	.ident	"GCC: (x86_64-posix-seh-rev0, Built by MinGW-Builds project) 15.2.0"
	.def	printf;	.scl	2;	.type	32;	.endef
	.def	puts;	.scl	2;	.type	32;	.endef
