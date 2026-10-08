	.file	"OPTBENCH.c"
	.intel_syntax noprefix
 # GNU C23 (x86_64-posix-seh-rev0, Built by MinGW-Builds project) version 15.2.0 (x86_64-w64-mingw32)
 #	compiled by GNU C version 15.2.0, GMP version 6.2.1, MPFR version 4.1.0, MPC version 1.2.1, isl version isl-0.27-GMP

 # GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
 # options passed: -masm=intel -mtune=core2 -march=nocona -O0
	.text
	.globl	i
	.bss
	.align 4
i:
	.space 4
	.globl	j
	.align 4
j:
	.space 4
	.globl	k
	.align 4
k:
	.space 4
	.globl	l
	.align 4
l:
	.space 4
	.globl	m
	.align 4
m:
	.space 4
	.globl	i2
	.align 4
i2:
	.space 4
	.globl	j2
	.align 4
j2:
	.space 4
	.globl	k2
	.align 4
k2:
	.space 4
	.globl	g3
	.align 4
g3:
	.space 4
	.globl	h3
	.align 4
h3:
	.space 4
	.globl	i3
	.align 4
i3:
	.space 4
	.globl	k3
	.align 4
k3:
	.space 4
	.globl	m3
	.align 4
m3:
	.space 4
	.globl	i4
	.align 4
i4:
	.space 4
	.globl	j4
	.align 4
j4:
	.space 4
	.globl	i5
	.align 4
i5:
	.space 4
	.globl	j5
	.align 4
j5:
	.space 4
	.globl	k5
	.align 4
k5:
	.space 4
	.globl	flt_1
	.align 8
flt_1:
	.space 8
	.globl	flt_2
	.align 8
flt_2:
	.space 8
	.globl	flt_3
	.align 8
flt_3:
	.space 8
	.globl	flt_4
	.align 8
flt_4:
	.space 8
	.globl	flt_5
	.align 8
flt_5:
	.space 8
	.globl	flt_6
	.align 8
flt_6:
	.space 8
	.globl	ivector
	.align 8
ivector:
	.space 12
	.globl	ivector2
ivector2:
	.space 3
	.globl	ivector4
	.align 8
ivector4:
	.space 12
	.globl	ivector5
	.align 32
ivector5:
	.space 400
	.section .rdata,"dr"
.LC0:
	.ascii "Hello\0"
	.align 8
.LC2:
	.ascii "Common subexpression elimination\0"
	.align 8
.LC3:
	.ascii "This line should not be printed\0"
	.text
	.globl	main
	.def	main;	.scl	2;	.type	32;	.endef
	.seh_proc	main
main:
	push	rbp	 #
	.seh_pushreg	rbp
	mov	rbp, rsp	 #,
	.seh_setframe	rbp, 0
	sub	rsp, 48	 #,
	.seh_stackalloc	48
	.seh_endprologue
	mov	DWORD PTR 16[rbp], ecx	 # argc, argc
	mov	QWORD PTR 24[rbp], rdx	 # argv, argv
 # OPTBENCH.c:53:          {
	call	__main	 #
 # OPTBENCH.c:57: 			 for(i = 0; i < 3 ; i++) ivector[ i ] = 1;
	mov	DWORD PTR i[rip], 0	 # i,
 # OPTBENCH.c:57: 			 for(i = 0; i < 3 ; i++) ivector[ i ] = 1;
	jmp	.L2	 #
.L3:
 # OPTBENCH.c:57: 			 for(i = 0; i < 3 ; i++) ivector[ i ] = 1;
	mov	eax, DWORD PTR i[rip]	 # i.0_1, i
 # OPTBENCH.c:57: 			 for(i = 0; i < 3 ; i++) ivector[ i ] = 1;
	cdqe
	lea	rdx, 0[0+rax*4]	 # tmp178,
	lea	rax, ivector[rip]	 # tmp179,
	mov	DWORD PTR [rdx+rax], 1	 # ivector[i.0_1],
 # OPTBENCH.c:57: 			 for(i = 0; i < 3 ; i++) ivector[ i ] = 1;
	mov	eax, DWORD PTR i[rip]	 # i.1_2, i
	add	eax, 1	 # _3,
	mov	DWORD PTR i[rip], eax	 # i, _3
.L2:
 # OPTBENCH.c:57: 			 for(i = 0; i < 3 ; i++) ivector[ i ] = 1;
	mov	eax, DWORD PTR i[rip]	 # i.2_4, i
	cmp	eax, 2	 # i.2_4,
	jle	.L3	 #,
 # OPTBENCH.c:62: 			 i2 = 5;
	mov	DWORD PTR i2[rip], 5	 # i2,
 # OPTBENCH.c:63: 			 j4 = 6;
	mov	DWORD PTR j4[rip], 6	 # j4,
 # OPTBENCH.c:64: 			 i2 = j4;
	mov	eax, DWORD PTR j4[rip]	 # j4.3_5, j4
	mov	DWORD PTR i2[rip], eax	 # i2, j4.3_5
 # OPTBENCH.c:69:             j4 = 2;
	mov	DWORD PTR j4[rip], 2	 # j4,
 # OPTBENCH.c:70:             if( i2 < j4 && i4 < j4 ){
	mov	edx, DWORD PTR i2[rip]	 # i2.4_6, i2
	mov	eax, DWORD PTR j4[rip]	 # j4.5_7, j4
 # OPTBENCH.c:70:             if( i2 < j4 && i4 < j4 ){
	cmp	edx, eax	 # i2.4_6, j4.5_7
	jge	.L4	 #,
 # OPTBENCH.c:70:             if( i2 < j4 && i4 < j4 ){
	mov	edx, DWORD PTR i4[rip]	 # i4.6_8, i4
	mov	eax, DWORD PTR j4[rip]	 # j4.7_9, j4
 # OPTBENCH.c:70:             if( i2 < j4 && i4 < j4 ){
	cmp	edx, eax	 # i4.6_8, j4.7_9
	jge	.L4	 #,
 # OPTBENCH.c:71:                 i2 = 2;
	mov	DWORD PTR i2[rip], 2	 # i2,
 # OPTBENCH.c:72: 				printf("Hello");
	lea	rax, .LC0[rip]	 # tmp180,
	mov	rcx, rax	 #, tmp180
	call	printf	 #
.L4:
 # OPTBENCH.c:75:             j4 = k5;
	mov	eax, DWORD PTR k5[rip]	 # k5.8_10, k5
	mov	DWORD PTR j4[rip], eax	 # j4, k5.8_10
 # OPTBENCH.c:76:             if( i2 < j4 && i4 < j4 ){
	mov	edx, DWORD PTR i2[rip]	 # i2.9_11, i2
	mov	eax, DWORD PTR j4[rip]	 # j4.10_12, j4
 # OPTBENCH.c:76:             if( i2 < j4 && i4 < j4 ){
	cmp	edx, eax	 # i2.9_11, j4.10_12
	jge	.L5	 #,
 # OPTBENCH.c:76:             if( i2 < j4 && i4 < j4 ){
	mov	edx, DWORD PTR i4[rip]	 # i4.11_13, i4
	mov	eax, DWORD PTR j4[rip]	 # j4.12_14, j4
 # OPTBENCH.c:76:             if( i2 < j4 && i4 < j4 ){
	cmp	edx, eax	 # i4.11_13, j4.12_14
	jge	.L5	 #,
 # OPTBENCH.c:77:                 i5 = 3;
	mov	DWORD PTR i5[rip], 3	 # i5,
 # OPTBENCH.c:78: 				printf("Hello");
	lea	rax, .LC0[rip]	 # tmp181,
	mov	rcx, rax	 #, tmp181
	call	printf	 #
.L5:
 # OPTBENCH.c:86:             i3 = 1 + 2;
	mov	DWORD PTR i3[rip], 3	 # i3,
 # OPTBENCH.c:87:             flt_1 = 2.4 + 6.3;
	movsd	xmm0, QWORD PTR .LC1[rip]	 # tmp182,
	movsd	QWORD PTR flt_1[rip], xmm0	 # flt_1, tmp182
 # OPTBENCH.c:88:             i2 = 5;
	mov	DWORD PTR i2[rip], 5	 # i2,
 # OPTBENCH.c:89:             j2 = i + 0;
	mov	eax, DWORD PTR i[rip]	 # i.13_15, i
	mov	DWORD PTR j2[rip], eax	 # j2, i.13_15
 # OPTBENCH.c:90:             k2 = i / 1;
	mov	eax, DWORD PTR i[rip]	 # i.14_16, i
	mov	DWORD PTR k2[rip], eax	 # k2, i.14_16
 # OPTBENCH.c:91:             i4 = i * 1;
	mov	eax, DWORD PTR i[rip]	 # i.15_17, i
	mov	DWORD PTR i4[rip], eax	 # i4, i.15_17
 # OPTBENCH.c:92:             i5 = i * 0;
	mov	DWORD PTR i5[rip], 0	 # i5,
 # OPTBENCH.c:114:             k3 = 1;
	mov	DWORD PTR k3[rip], 1	 # k3,
 # OPTBENCH.c:115:             k3 = 1;
	mov	DWORD PTR k3[rip], 1	 # k3,
 # OPTBENCH.c:121: 	    k2 = 4 * j5;
	mov	eax, DWORD PTR j5[rip]	 # j5.16_18, j5
	sal	eax, 2	 # _19,
 # OPTBENCH.c:121: 	    k2 = 4 * j5;
	mov	DWORD PTR k2[rip], eax	 # k2, _19
 # OPTBENCH.c:122: 	    for( i = 0; i <= 5; i++ )
	mov	DWORD PTR i[rip], 0	 # i,
 # OPTBENCH.c:122: 	    for( i = 0; i <= 5; i++ )
	jmp	.L6	 #
.L7:
 # OPTBENCH.c:123: 		ivector4[ i ] = i * 2;
	mov	eax, DWORD PTR i[rip]	 # i.17_20, i
	lea	edx, [rax+rax]	 # _22,
 # OPTBENCH.c:123: 		ivector4[ i ] = i * 2;
	mov	eax, DWORD PTR i[rip]	 # i.18_23, i
 # OPTBENCH.c:123: 		ivector4[ i ] = i * 2;
	mov	ecx, edx	 # _24, _22
	cdqe
	lea	rdx, [rax+rax]	 # tmp184,
	lea	rax, ivector4[rip]	 # tmp185,
	mov	WORD PTR [rdx+rax], cx	 # ivector4[i.18_23], _24
 # OPTBENCH.c:122: 	    for( i = 0; i <= 5; i++ )
	mov	eax, DWORD PTR i[rip]	 # i.19_25, i
	add	eax, 1	 # _26,
	mov	DWORD PTR i[rip], eax	 # i, _26
.L6:
 # OPTBENCH.c:122: 	    for( i = 0; i <= 5; i++ )
	mov	eax, DWORD PTR i[rip]	 # i.20_27, i
	cmp	eax, 5	 # i.20_27,
	jle	.L7	 #,
 # OPTBENCH.c:129:             j5 = 0;
	mov	DWORD PTR j5[rip], 0	 # j5,
 # OPTBENCH.c:130:             k5 = 10000;
	mov	DWORD PTR k5[rip], 10000	 # k5,
.L8:
 # OPTBENCH.c:132:                 k5 = k5 - 1;
	mov	eax, DWORD PTR k5[rip]	 # k5.21_28, k5
	sub	eax, 1	 # _29,
 # OPTBENCH.c:132:                 k5 = k5 - 1;
	mov	DWORD PTR k5[rip], eax	 # k5, _29
 # OPTBENCH.c:133:                 j5 = j5 + 1;
	mov	eax, DWORD PTR j5[rip]	 # j5.22_30, j5
	add	eax, 1	 # _31,
 # OPTBENCH.c:133:                 j5 = j5 + 1;
	mov	DWORD PTR j5[rip], eax	 # j5, _31
 # OPTBENCH.c:134:                 i5 = (k5 * 3) / (j5 * constant5);
	mov	edx, DWORD PTR k5[rip]	 # k5.23_32, k5
	mov	eax, edx	 # tmp186, k5.23_32
	add	eax, eax	 # tmp186
	lea	ecx, [rax+rdx]	 # _33,
 # OPTBENCH.c:134:                 i5 = (k5 * 3) / (j5 * constant5);
	mov	edx, DWORD PTR j5[rip]	 # j5.24_34, j5
	mov	eax, edx	 # tmp187, j5.24_34
	sal	eax, 2	 # tmp187,
	lea	r10d, [rdx+rax]	 # _35,
 # OPTBENCH.c:134:                 i5 = (k5 * 3) / (j5 * constant5);
	mov	eax, ecx	 # _33, _33
	cdq
	idiv	r10d	 # _35
 # OPTBENCH.c:134:                 i5 = (k5 * 3) / (j5 * constant5);
	mov	DWORD PTR i5[rip], eax	 # i5, _36
 # OPTBENCH.c:135:                } while ( k5 > 0 );
	mov	eax, DWORD PTR k5[rip]	 # k5.25_37, k5
	test	eax, eax	 # k5.25_37
	jg	.L8	 #,
 # OPTBENCH.c:140:             for( i = 0; i < 100; i++ )
	mov	DWORD PTR i[rip], 0	 # i,
 # OPTBENCH.c:140:             for( i = 0; i < 100; i++ )
	jmp	.L9	 #
.L10:
 # OPTBENCH.c:141:                 ivector5[ i * 2 + 3 ] = 5;
	mov	eax, DWORD PTR i[rip]	 # i.26_38, i
	add	eax, eax	 # _39
 # OPTBENCH.c:141:                 ivector5[ i * 2 + 3 ] = 5;
	add	eax, 3	 # _40,
 # OPTBENCH.c:141:                 ivector5[ i * 2 + 3 ] = 5;
	cdqe
	lea	rdx, 0[0+rax*4]	 # tmp191,
	lea	rax, ivector5[rip]	 # tmp192,
	mov	DWORD PTR [rdx+rax], 5	 # ivector5[_40],
 # OPTBENCH.c:140:             for( i = 0; i < 100; i++ )
	mov	eax, DWORD PTR i[rip]	 # i.27_41, i
	add	eax, 1	 # _42,
	mov	DWORD PTR i[rip], eax	 # i, _42
.L9:
 # OPTBENCH.c:140:             for( i = 0; i < 100; i++ )
	mov	eax, DWORD PTR i[rip]	 # i.28_43, i
	cmp	eax, 99	 # i.28_43,
	jle	.L10	 #,
 # OPTBENCH.c:147:             if( i < 10 )
	mov	eax, DWORD PTR i[rip]	 # i.29_44, i
 # OPTBENCH.c:147:             if( i < 10 )
	cmp	eax, 9	 # i.29_44,
	jg	.L11	 #,
 # OPTBENCH.c:148:                 j5 = i5 + i2;
	mov	edx, DWORD PTR i5[rip]	 # i5.30_45, i5
	mov	eax, DWORD PTR i2[rip]	 # i2.31_46, i2
	add	eax, edx	 # _47, i5.30_45
 # OPTBENCH.c:148:                 j5 = i5 + i2;
	mov	DWORD PTR j5[rip], eax	 # j5, _47
	jmp	.L12	 #
.L11:
 # OPTBENCH.c:150:                 k5 = i5 + i2;
	mov	edx, DWORD PTR i5[rip]	 # i5.32_48, i5
	mov	eax, DWORD PTR i2[rip]	 # i2.33_49, i2
	add	eax, edx	 # _50, i5.32_48
 # OPTBENCH.c:150:                 k5 = i5 + i2;
	mov	DWORD PTR k5[rip], eax	 # k5, _50
.L12:
 # OPTBENCH.c:158:             ivector[ 0 ] = 1;  /* генерация константного адреса */
	mov	DWORD PTR ivector[rip], 1	 # ivector[0],
 # OPTBENCH.c:159:             ivector[ i2 ] = 2; /* значение i2 должно быть скопировано*/
	mov	eax, DWORD PTR i2[rip]	 # i2.34_51, i2
 # OPTBENCH.c:159:             ivector[ i2 ] = 2; /* значение i2 должно быть скопировано*/
	cdqe
	lea	rdx, 0[0+rax*4]	 # tmp194,
	lea	rax, ivector[rip]	 # tmp195,
	mov	DWORD PTR [rdx+rax], 2	 # ivector[i2.34_51],
 # OPTBENCH.c:160:             ivector[ i2 ] = 2; /* копирование регистров */
	mov	eax, DWORD PTR i2[rip]	 # i2.35_52, i2
 # OPTBENCH.c:160:             ivector[ i2 ] = 2; /* копирование регистров */
	cdqe
	lea	rdx, 0[0+rax*4]	 # tmp197,
	lea	rax, ivector[rip]	 # tmp198,
	mov	DWORD PTR [rdx+rax], 2	 # ivector[i2.35_52],
 # OPTBENCH.c:161:             ivector[ 2 ] = 3;  /* генарация константного адреса */
	mov	DWORD PTR ivector[rip+8], 3	 # ivector[2],
 # OPTBENCH.c:168:             if(( h3 + k3 ) < 0 || ( h3 + k3 ) > 5 )
	mov	edx, DWORD PTR h3[rip]	 # h3.36_53, h3
	mov	eax, DWORD PTR k3[rip]	 # k3.37_54, k3
	add	eax, edx	 # _55, h3.36_53
 # OPTBENCH.c:168:             if(( h3 + k3 ) < 0 || ( h3 + k3 ) > 5 )
	test	eax, eax	 # _55
	js	.L13	 #,
 # OPTBENCH.c:168:             if(( h3 + k3 ) < 0 || ( h3 + k3 ) > 5 )
	mov	edx, DWORD PTR h3[rip]	 # h3.38_56, h3
	mov	eax, DWORD PTR k3[rip]	 # k3.39_57, k3
	add	eax, edx	 # _58, h3.38_56
 # OPTBENCH.c:168:             if(( h3 + k3 ) < 0 || ( h3 + k3 ) > 5 )
	cmp	eax, 5	 # _58,
	jle	.L14	 #,
.L13:
 # OPTBENCH.c:169:                 printf("Common subexpression elimination\n");
	lea	rax, .LC2[rip]	 # tmp199,
	mov	rcx, rax	 #, tmp199
	call	puts	 #
	jmp	.L15	 #
.L14:
 # OPTBENCH.c:171:                 m3 = ( h3 + k3 ) / i3;
	mov	edx, DWORD PTR h3[rip]	 # h3.40_59, h3
	mov	eax, DWORD PTR k3[rip]	 # k3.41_60, k3
	add	eax, edx	 # _61, h3.40_59
 # OPTBENCH.c:171:                 m3 = ( h3 + k3 ) / i3;
	mov	ecx, DWORD PTR i3[rip]	 # i3.42_62, i3
	cdq
	idiv	ecx	 # i3.42_62
 # OPTBENCH.c:171:                 m3 = ( h3 + k3 ) / i3;
	mov	DWORD PTR m3[rip], eax	 # m3, _63
 # OPTBENCH.c:172: 		g3 = i3 + (h3 + k3);
	mov	edx, DWORD PTR h3[rip]	 # h3.43_64, h3
	mov	eax, DWORD PTR k3[rip]	 # k3.44_65, k3
	add	edx, eax	 # _66, k3.44_65
 # OPTBENCH.c:172: 		g3 = i3 + (h3 + k3);
	mov	eax, DWORD PTR i3[rip]	 # i3.45_67, i3
	add	eax, edx	 # _68, _66
 # OPTBENCH.c:172: 		g3 = i3 + (h3 + k3);
	mov	DWORD PTR g3[rip], eax	 # g3, _68
.L15:
 # OPTBENCH.c:180:             for( i4 = 0; i4 <= max_vector; i4++){
	mov	DWORD PTR i4[rip], 0	 # i4,
 # OPTBENCH.c:180:             for( i4 = 0; i4 <= max_vector; i4++){
	jmp	.L16	 #
.L17:
 # OPTBENCH.c:181: 				printf("Hello");
	lea	rax, .LC0[rip]	 # tmp202,
	mov	rcx, rax	 #, tmp202
	call	printf	 #
 # OPTBENCH.c:182:                 ivector2[ i4 ] = j * k;
	mov	eax, DWORD PTR j[rip]	 # j.46_69, j
	mov	edx, DWORD PTR k[rip]	 # k.47_71, k
	mov	ecx, edx	 # _72, k.47_71
 # OPTBENCH.c:182:                 ivector2[ i4 ] = j * k;
	mov	edx, DWORD PTR i4[rip]	 # i4.48_73, i4
 # OPTBENCH.c:182:                 ivector2[ i4 ] = j * k;
	imul	eax, ecx	 # _74, _72
	movsx	rdx, edx	 # tmp203, i4.48_73
	lea	rcx, ivector2[rip]	 # tmp204,
	mov	BYTE PTR [rdx+rcx], al	 # ivector2[i4.48_73], _74
 # OPTBENCH.c:180:             for( i4 = 0; i4 <= max_vector; i4++){
	mov	eax, DWORD PTR i4[rip]	 # i4.49_75, i4
	add	eax, 1	 # _76,
	mov	DWORD PTR i4[rip], eax	 # i4, _76
.L16:
 # OPTBENCH.c:180:             for( i4 = 0; i4 <= max_vector; i4++){
	mov	eax, DWORD PTR i4[rip]	 # i4.50_77, i4
	cmp	eax, 2	 # i4.50_77,
	jle	.L17	 #,
 # OPTBENCH.c:189: 	    dead_code( 1, "This line should not be printed" );
	lea	rax, .LC3[rip]	 # tmp205,
	mov	rdx, rax	 #, tmp205
	mov	ecx, 1	 #,
	call	dead_code	 #
 # OPTBENCH.c:195:             unnecessary_loop();
	call	unnecessary_loop	 #
 # OPTBENCH.c:198: 	loop_jamming(7);
	mov	ecx, 7	 #,
	call	loop_jamming	 #
 # OPTBENCH.c:199: 	loop_unrolling(7);
	mov	ecx, 7	 #,
	call	loop_unrolling	 #
 # OPTBENCH.c:200: 	jump_compression(1, 2, 3, 4, 5);
	mov	DWORD PTR 32[rsp], 5	 #,
	mov	r9d, 4	 #,
	mov	r8d, 3	 #,
	mov	edx, 2	 #,
	mov	ecx, 1	 #,
	call	jump_compression	 #
	mov	eax, 0	 # _130,
 # OPTBENCH.c:202:          }    /* Конец функции main */
	add	rsp, 48	 #,
	pop	rbp	 #
	ret	
	.seh_endproc
	.globl	dead_code
	.def	dead_code;	.scl	2;	.type	32;	.endef
	.seh_proc	dead_code
dead_code:
	push	rbp	 #
	.seh_pushreg	rbp
	mov	rbp, rsp	 #,
	.seh_setframe	rbp, 0
	sub	rsp, 16	 #,
	.seh_stackalloc	16
	.seh_endprologue
	mov	DWORD PTR 16[rbp], ecx	 # a, a
	mov	QWORD PTR 24[rbp], rdx	 # b, b
 # OPTBENCH.c:217:               idead_store = a;
	mov	eax, DWORD PTR 16[rbp]	 # tmp98, a
	mov	DWORD PTR -4[rbp], eax	 # idead_store, tmp98
 # OPTBENCH.c:220:             } /* Конец dead_code */
	nop	
	add	rsp, 16	 #,
	pop	rbp	 #
	ret	
	.seh_endproc
	.globl	unnecessary_loop
	.def	unnecessary_loop;	.scl	2;	.type	32;	.endef
	.seh_proc	unnecessary_loop
unnecessary_loop:
	push	rbp	 #
	.seh_pushreg	rbp
	mov	rbp, rsp	 #,
	.seh_setframe	rbp, 0
	sub	rsp, 16	 #,
	.seh_stackalloc	16
	.seh_endprologue
 # OPTBENCH.c:234:               x = 0;
	mov	DWORD PTR -4[rbp], 0	 # x,
 # OPTBENCH.c:235:               for( i = 0; i < 5; i++ )  /* Цикл не должен
	mov	DWORD PTR i[rip], 0	 # i,
 # OPTBENCH.c:235:               for( i = 0; i < 5; i++ )  /* Цикл не должен
	jmp	.L21	 #
.L22:
 # OPTBENCH.c:237:                   k5 = x + j5;
	mov	edx, DWORD PTR j5[rip]	 # j5.51_1, j5
	mov	eax, DWORD PTR -4[rbp]	 # tmp103, x
	add	eax, edx	 # _2, j5.51_1
 # OPTBENCH.c:237:                   k5 = x + j5;
	mov	DWORD PTR k5[rip], eax	 # k5, _2
 # OPTBENCH.c:235:               for( i = 0; i < 5; i++ )  /* Цикл не должен
	mov	eax, DWORD PTR i[rip]	 # i.52_3, i
	add	eax, 1	 # _4,
	mov	DWORD PTR i[rip], eax	 # i, _4
.L21:
 # OPTBENCH.c:235:               for( i = 0; i < 5; i++ )  /* Цикл не должен
	mov	eax, DWORD PTR i[rip]	 # i.53_5, i
	cmp	eax, 4	 # i.53_5,
	jle	.L22	 #,
 # OPTBENCH.c:238:             } /* Конец unnecessary_loop */
	nop	
	nop	
	add	rsp, 16	 #,
	pop	rbp	 #
	ret	
	.seh_endproc
	.globl	loop_jamming
	.def	loop_jamming;	.scl	2;	.type	32;	.endef
	.seh_proc	loop_jamming
loop_jamming:
	push	rbp	 #
	.seh_pushreg	rbp
	mov	rbp, rsp	 #,
	.seh_setframe	rbp, 0
	.seh_endprologue
	mov	DWORD PTR 16[rbp], ecx	 # x, x
 # OPTBENCH.c:250:                 for( i = 0; i < 5; i++ )
	mov	DWORD PTR i[rip], 0	 # i,
 # OPTBENCH.c:250:                 for( i = 0; i < 5; i++ )
	jmp	.L24	 #
.L25:
 # OPTBENCH.c:251:                     k5 = x + j5 * i;
	mov	edx, DWORD PTR j5[rip]	 # j5.54_1, j5
	mov	eax, DWORD PTR i[rip]	 # i.55_2, i
	imul	eax, edx	 # _3, j5.54_1
 # OPTBENCH.c:251:                     k5 = x + j5 * i;
	mov	edx, DWORD PTR 16[rbp]	 # tmp112, x
	add	eax, edx	 # _4, tmp112
 # OPTBENCH.c:251:                     k5 = x + j5 * i;
	mov	DWORD PTR k5[rip], eax	 # k5, _4
 # OPTBENCH.c:250:                 for( i = 0; i < 5; i++ )
	mov	eax, DWORD PTR i[rip]	 # i.56_5, i
	add	eax, 1	 # _6,
	mov	DWORD PTR i[rip], eax	 # i, _6
.L24:
 # OPTBENCH.c:250:                 for( i = 0; i < 5; i++ )
	mov	eax, DWORD PTR i[rip]	 # i.57_7, i
	cmp	eax, 4	 # i.57_7,
	jle	.L25	 #,
 # OPTBENCH.c:252:                 for( i = 0; i < 5; i++ )
	mov	DWORD PTR i[rip], 0	 # i,
 # OPTBENCH.c:252:                 for( i = 0; i < 5; i++ )
	jmp	.L26	 #
.L27:
 # OPTBENCH.c:253:                     i5 = x * k5 * i;
	mov	eax, DWORD PTR k5[rip]	 # k5.58_8, k5
	imul	eax, DWORD PTR 16[rbp]	 # _9, x
 # OPTBENCH.c:253:                     i5 = x * k5 * i;
	mov	edx, DWORD PTR i[rip]	 # i.59_10, i
	imul	eax, edx	 # _11, i.59_10
 # OPTBENCH.c:253:                     i5 = x * k5 * i;
	mov	DWORD PTR i5[rip], eax	 # i5, _11
 # OPTBENCH.c:252:                 for( i = 0; i < 5; i++ )
	mov	eax, DWORD PTR i[rip]	 # i.60_12, i
	add	eax, 1	 # _13,
	mov	DWORD PTR i[rip], eax	 # i, _13
.L26:
 # OPTBENCH.c:252:                 for( i = 0; i < 5; i++ )
	mov	eax, DWORD PTR i[rip]	 # i.61_14, i
	cmp	eax, 4	 # i.61_14,
	jle	.L27	 #,
 # OPTBENCH.c:254:             } /* Конец loop_jamming */
	nop	
	nop	
	pop	rbp	 #
	ret	
	.seh_endproc
	.globl	loop_unrolling
	.def	loop_unrolling;	.scl	2;	.type	32;	.endef
	.seh_proc	loop_unrolling
loop_unrolling:
	push	rbp	 #
	.seh_pushreg	rbp
	mov	rbp, rsp	 #,
	.seh_setframe	rbp, 0
	.seh_endprologue
	mov	DWORD PTR 16[rbp], ecx	 # x, x
 # OPTBENCH.c:268:                 for( i = 0; i < 6; i++ )
	mov	DWORD PTR i[rip], 0	 # i,
 # OPTBENCH.c:268:                 for( i = 0; i < 6; i++ )
	jmp	.L29	 #
.L30:
 # OPTBENCH.c:269:                     ivector4[ i ] = 0;
	mov	eax, DWORD PTR i[rip]	 # i.62_1, i
 # OPTBENCH.c:269:                     ivector4[ i ] = 0;
	cdqe
	lea	rdx, [rax+rax]	 # tmp103,
	lea	rax, ivector4[rip]	 # tmp104,
	mov	WORD PTR [rdx+rax], 0	 # ivector4[i.62_1],
 # OPTBENCH.c:268:                 for( i = 0; i < 6; i++ )
	mov	eax, DWORD PTR i[rip]	 # i.63_2, i
	add	eax, 1	 # _3,
	mov	DWORD PTR i[rip], eax	 # i, _3
.L29:
 # OPTBENCH.c:268:                 for( i = 0; i < 6; i++ )
	mov	eax, DWORD PTR i[rip]	 # i.64_4, i
	cmp	eax, 5	 # i.64_4,
	jle	.L30	 #,
 # OPTBENCH.c:270: 	    } /* Конец loop_unrolling */
	nop	
	nop	
	pop	rbp	 #
	ret	
	.seh_endproc
	.globl	jump_compression
	.def	jump_compression;	.scl	2;	.type	32;	.endef
	.seh_proc	jump_compression
jump_compression:
	push	rbp	 #
	.seh_pushreg	rbp
	mov	rbp, rsp	 #,
	.seh_setframe	rbp, 0
	.seh_endprologue
	mov	DWORD PTR 16[rbp], ecx	 # i, i
	mov	DWORD PTR 24[rbp], edx	 # j, j
	mov	DWORD PTR 32[rbp], r8d	 # k, k
	mov	DWORD PTR 40[rbp], r9d	 # l, l
.L32:
 # OPTBENCH.c:283:                if( i < j )
	mov	eax, DWORD PTR 16[rbp]	 # tmp103, i
	cmp	eax, DWORD PTR 24[rbp]	 # tmp103, j
	jge	.L33	 #,
 # OPTBENCH.c:284:                    if( j < k )
	mov	eax, DWORD PTR 24[rbp]	 # tmp104, j
	cmp	eax, DWORD PTR 32[rbp]	 # tmp104, k
	jge	.L34	 #,
 # OPTBENCH.c:285:                        if( k < l )
	mov	eax, DWORD PTR 32[rbp]	 # tmp105, k
	cmp	eax, DWORD PTR 40[rbp]	 # tmp105, l
	jge	.L35	 #,
 # OPTBENCH.c:286: 			   if( l < m )
	mov	eax, DWORD PTR 40[rbp]	 # tmp106, l
	cmp	eax, DWORD PTR 48[rbp]	 # tmp106, m
	jge	.L40	 #,
 # OPTBENCH.c:287:                                l += m;
	mov	eax, DWORD PTR 48[rbp]	 # tmp107, m
	add	DWORD PTR 40[rbp], eax	 # l, tmp107
	jmp	.L37	 #
.L35:
 # OPTBENCH.c:291:                            k += l;
	mov	eax, DWORD PTR 40[rbp]	 # tmp108, l
	add	DWORD PTR 32[rbp], eax	 # k, tmp108
	jmp	.L37	 #
.L34:
 # OPTBENCH.c:293:                        j += k;
	mov	eax, DWORD PTR 32[rbp]	 # tmp109, k
	add	DWORD PTR 24[rbp], eax	 # j, tmp109
	jmp	.L32	 #
.L40:
 # OPTBENCH.c:289:                                goto end_1;
	nop	
.L38:
 # OPTBENCH.c:295:                        goto beg_1;
	jmp	.L32	 #
.L33:
 # OPTBENCH.c:298:                    i += j;
	mov	eax, DWORD PTR 24[rbp]	 # tmp110, j
	add	DWORD PTR 16[rbp], eax	 # i, tmp110
.L37:
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	mov	edx, DWORD PTR 16[rbp]	 # tmp111, i
	mov	eax, DWORD PTR 24[rbp]	 # tmp112, j
	add	edx, eax	 # _1, tmp112
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	mov	eax, DWORD PTR 32[rbp]	 # tmp113, k
	add	edx, eax	 # _2, tmp113
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	mov	eax, DWORD PTR 40[rbp]	 # tmp114, l
	add	edx, eax	 # _3, tmp114
 # OPTBENCH.c:299:                return( i + j + k + l + m );
	mov	eax, DWORD PTR 48[rbp]	 # tmp115, m
	add	eax, edx	 # _18, _3
 # OPTBENCH.c:300:            } /* Конец jump_compression */
	pop	rbp	 #
	ret	
	.seh_endproc
	.section .rdata,"dr"
	.align 8
.LC1:
	.long	1717986918
	.long	1075930726
	.def	__main;	.scl	2;	.type	32;	.endef
	.ident	"GCC: (x86_64-posix-seh-rev0, Built by MinGW-Builds project) 15.2.0"
	.def	printf;	.scl	2;	.type	32;	.endef
	.def	puts;	.scl	2;	.type	32;	.endef
