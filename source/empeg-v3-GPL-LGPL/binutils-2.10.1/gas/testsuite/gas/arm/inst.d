#objdump: -dr --prefix-addresses --show-raw-insn
#name: ARM basic instructions
#as: -marm2 -EL

# Test the standard ARM instructions:

.*: +file format .*arm.*

Disassembly of section .text:
00000000 <[^>]*> e3a00000 ?	mov	r0, #0	; 0x0
00000004 <[^>]*> e1a01002 ?	mov	r1, r2
00000008 <[^>]*> e1a03184 ?	mov	r3, r4, lsl #3
0000000c <[^>]*> e1a05736 ?	mov	r5, r6, lsr r7
00000010 <[^>]*> e1a08a59 ?	mov	r8, r9, asr r10
00000014 <[^>]*> e1a0bd1c ?	mov	r11, r12, lsl sp
00000018 <[^>]*> e1a0e06f ?	mov	lr, pc, rrx
0000001c <[^>]*> e1a01002 ?	mov	r1, r2
00000020 <[^>]*> 01a02003 ?	moveq	r2, r3
00000024 <[^>]*> 11a04005 ?	movne	r4, r5
00000028 <[^>]*> b1a06007 ?	movlt	r6, r7
0000002c <[^>]*> a1a08009 ?	movge	r8, r9
00000030 <[^>]*> d1a0a00b ?	movle	r10, r11
00000034 <[^>]*> c1a0c00d ?	movgt	r12, sp
00000038 <[^>]*> 31a01002 ?	movcc	r1, r2
0000003c <[^>]*> 21a01003 ?	movcs	r1, r3
00000040 <[^>]*> 41a03006 ?	movmi	r3, r6
00000044 <[^>]*> 51a07009 ?	movpl	r7, r9
00000048 <[^>]*> 61a01008 ?	movvs	r1, r8
0000004c <[^>]*> 71a09fa1 ?	movvc	r9, r1, lsr #31
00000050 <[^>]*> 81a0800f ?	movhi	r8, pc
00000054 <[^>]*> 91a0f00e ?	movls	pc, lr
00000058 <[^>]*> 21a09008 ?	movcs	r9, r8
0000005c <[^>]*> 31a01003 ?	movcc	r1, r3
00000060 <[^>]*> e1b00008 ?	movs	r0, r8
00000064 <[^>]*> 31b00007 ?	movccs	r0, r7
00000068 <[^>]*> e281000a ?	add	r0, r1, #10	; 0xa
0000006c <[^>]*> e0832004 ?	add	r2, r3, r4
00000070 <[^>]*> e0865287 ?	add	r5, r6, r7, lsl #5
00000074 <[^>]*> e0821113 ?	add	r1, r2, r3, lsl r1
00000078 <[^>]*> e201000a ?	and	r0, r1, #10	; 0xa
0000007c <[^>]*> e0032004 ?	and	r2, r3, r4
00000080 <[^>]*> e0065287 ?	and	r5, r6, r7, lsl #5
00000084 <[^>]*> e0021113 ?	and	r1, r2, r3, lsl r1
00000088 <[^>]*> e221000a ?	eor	r0, r1, #10	; 0xa
0000008c <[^>]*> e0232004 ?	eor	r2, r3, r4
00000090 <[^>]*> e0265287 ?	eor	r5, r6, r7, lsl #5
00000094 <[^>]*> e0221113 ?	eor	r1, r2, r3, lsl r1
00000098 <[^>]*> e241000a ?	sub	r0, r1, #10	; 0xa
0000009c <[^>]*> e0432004 ?	sub	r2, r3, r4
000000a0 <[^>]*> e0465287 ?	sub	r5, r6, r7, lsl #5
000000a4 <[^>]*> e0421113 ?	sub	r1, r2, r3, lsl r1
000000a8 <[^>]*> e2a1000a ?	adc	r0, r1, #10	; 0xa
000000ac <[^>]*> e0a32004 ?	adc	r2, r3, r4
000000b0 <[^>]*> e0a65287 ?	adc	r5, r6, r7, lsl #5
000000b4 <[^>]*> e0a21113 ?	adc	r1, r2, r3, lsl r1
000000b8 <[^>]*> e2c1000a ?	sbc	r0, r1, #10	; 0xa
000000bc <[^>]*> e0c32004 ?	sbc	r2, r3, r4
000000c0 <[^>]*> e0c65287 ?	sbc	r5, r6, r7, lsl #5
000000c4 <[^>]*> e0c21113 ?	sbc	r1, r2, r3, lsl r1
000000c8 <[^>]*> e261000a ?	rsb	r0, r1, #10	; 0xa
000000cc <[^>]*> e0632004 ?	rsb	r2, r3, r4
000000d0 <[^>]*> e0665287 ?	rsb	r5, r6, r7, lsl #5
000000d4 <[^>]*> e0621113 ?	rsb	r1, r2, r3, lsl r1
000000d8 <[^>]*> e2e1000a ?	rsc	r0, r1, #10	; 0xa
000000dc <[^>]*> e0e32004 ?	rsc	r2, r3, r4
000000e0 <[^>]*> e0e65287 ?	rsc	r5, r6, r7, lsl #5
000000e4 <[^>]*> e0e21113 ?	rsc	r1, r2, r3, lsl r1
000000e8 <[^>]*> e381000a ?	orr	r0, r1, #10	; 0xa
000000ec <[^>]*> e1832004 ?	orr	r2, r3, r4
000000f0 <[^>]*> e1865287 ?	orr	r5, r6, r7, lsl #5
000000f4 <[^>]*> e1821113 ?	orr	r1, r2, r3, lsl r1
000000f8 <[^>]*> e3c1000a ?	bic	r0, r1, #10	; 0xa
000000fc <[^>]*> e1c32004 ?	bic	r2, r3, r4
00000100 <[^>]*> e1c65287 ?	bic	r5, r6, r7, lsl #5
00000104 <[^>]*> e1c21113 ?	bic	r1, r2, r3, lsl r1
00000108 <[^>]*> e3e0000a ?	mvn	r0, #10	; 0xa
0000010c <[^>]*> e1e02004 ?	mvn	r2, r4
00000110 <[^>]*> e1e05287 ?	mvn	r5, r7, lsl #5
00000114 <[^>]*> e1e01113 ?	mvn	r1, r3, lsl r1
00000118 <[^>]*> e310000a ?	tst	r0, #10	; 0xa
0000011c <[^>]*> e1120004 ?	tst	r2, r4
00000120 <[^>]*> e1150287 ?	tst	r5, r7, lsl #5
00000124 <[^>]*> e1110113 ?	tst	r1, r3, lsl r1
00000128 <[^>]*> e330000a ?	teq	r0, #10	; 0xa
0000012c <[^>]*> e1320004 ?	teq	r2, r4
00000130 <[^>]*> e1350287 ?	teq	r5, r7, lsl #5
00000134 <[^>]*> e1310113 ?	teq	r1, r3, lsl r1
00000138 <[^>]*> e350000a ?	cmp	r0, #10	; 0xa
0000013c <[^>]*> e1520004 ?	cmp	r2, r4
00000140 <[^>]*> e1550287 ?	cmp	r5, r7, lsl #5
00000144 <[^>]*> e1510113 ?	cmp	r1, r3, lsl r1
00000148 <[^>]*> e370000a ?	cmn	r0, #10	; 0xa
0000014c <[^>]*> e1720004 ?	cmn	r2, r4
00000150 <[^>]*> e1750287 ?	cmn	r5, r7, lsl #5
00000154 <[^>]*> e1710113 ?	cmn	r1, r3, lsl r1
00000158 <[^>]*> e330f00a ?	teqp	r0, #10	; 0xa
0000015c <[^>]*> e132f004 ?	teqp	r2, r4
00000160 <[^>]*> e135f287 ?	teqp	r5, r7, lsl #5
00000164 <[^>]*> e131f113 ?	teqp	r1, r3, lsl r1
00000168 <[^>]*> e370f00a ?	cmnp	r0, #10	; 0xa
0000016c <[^>]*> e172f004 ?	cmnp	r2, r4
00000170 <[^>]*> e175f287 ?	cmnp	r5, r7, lsl #5
00000174 <[^>]*> e171f113 ?	cmnp	r1, r3, lsl r1
00000178 <[^>]*> e350f00a ?	cmpp	r0, #10	; 0xa
0000017c <[^>]*> e152f004 ?	cmpp	r2, r4
00000180 <[^>]*> e155f287 ?	cmpp	r5, r7, lsl #5
00000184 <[^>]*> e151f113 ?	cmpp	r1, r3, lsl r1
00000188 <[^>]*> e310f00a ?	tstp	r0, #10	; 0xa
0000018c <[^>]*> e112f004 ?	tstp	r2, r4
00000190 <[^>]*> e115f287 ?	tstp	r5, r7, lsl #5
00000194 <[^>]*> e111f113 ?	tstp	r1, r3, lsl r1
00000198 <[^>]*> e0000291 ?	mul	r0, r1, r2
0000019c <[^>]*> e0110392 ?	muls	r1, r2, r3
000001a0 <[^>]*> 10000091 ?	mulne	r0, r1, r0
000001a4 <[^>]*> 90190798 ?	mullss	r9, r8, r7
000001a8 <[^>]*> e021ba99 ?	mla	r1, r9, r10, r11
000001ac <[^>]*> e033c994 ?	mlas	r3, r4, r9, r12
000001b0 <[^>]*> b029d798 ?	mlalt	r9, r8, r7, sp
000001b4 <[^>]*> a034e391 ?	mlages	r4, r1, r3, lr
000001b8 <[^>]*> e5910000 ?	ldr	r0, \[r1\]
000001bc <[^>]*> e7911002 ?	ldr	r1, \[r1, r2\]
000001c0 <[^>]*> e7b32004 ?	ldr	r2, \[r3, r4\]!
000001c4 <[^>]*> e5922020 ?	ldr	r2, \[r2, #32\]
000001c8 <[^>]*> e7932424 ?	ldr	r2, \[r3, r4, lsr #8\]
000001cc <[^>]*> 07b54484 ?	ldreq	r4, \[r5, r4, lsl #9\]!
000001d0 <[^>]*> 14954006 ?	ldrne	r4, \[r5\], #6
000001d4 <[^>]*> e6b21003 ?	ldrt	r1, \[r2\], r3
000001d8 <[^>]*> e6942425 ?	ldr	r2, \[r4\], r5, lsr #8
000001dc <[^>]*> e51f0008 ?	ldr	r0, \[pc, #fffffff8\]	; 000001dc <[^>]*>
000001e0 <[^>]*> e5d43000 ?	ldrb	r3, \[r4\]
000001e4 <[^>]*> 14f85000 ?	ldrnebt	r5, \[r8\]
000001e8 <[^>]*> e5810000 ?	str	r0, \[r1\]
000001ec <[^>]*> e7811002 ?	str	r1, \[r1, r2\]
000001f0 <[^>]*> e7a43003 ?	str	r3, \[r4, r3\]!
000001f4 <[^>]*> e5822020 ?	str	r2, \[r2, #32\]
000001f8 <[^>]*> e7832424 ?	str	r2, \[r3, r4, lsr #8\]
000001fc <[^>]*> 07a54484 ?	streq	r4, \[r5, r4, lsl #9\]!
00000200 <[^>]*> 14854006 ?	strne	r4, \[r5\], #6
00000204 <[^>]*> e6821003 ?	str	r1, \[r2\], r3
00000208 <[^>]*> e6a42425 ?	strt	r2, \[r4\], r5, lsr #8
0000020c <[^>]*> e50f1004 ?	str	r1, \[pc, #fffffffc\]	; 00000210 <[^>]*>
00000210 <[^>]*> e5c71000 ?	strb	r1, \[r7\]
00000214 <[^>]*> e4e02000 ?	strbt	r2, \[r0\]
00000218 <[^>]*> e8900002 ?	ldmia	r0, {r1}
0000021c <[^>]*> 09920038 ?	ldmeqib	r2, {r3, r4, r5}
00000220 <[^>]*> e853ffff ?	ldmda	r3, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr, pc}\^
00000224 <[^>]*> e93b05ff ?	ldmdb	r11!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r10}
00000228 <[^>]*> e99100f7 ?	ldmib	r1, {r0, r1, r2, r4, r5, r6, r7}
0000022c <[^>]*> e89201f8 ?	ldmia	r2, {r3, r4, r5, r6, r7, r8}
00000230 <[^>]*> e9130003 ?	ldmdb	r3, {r0, r1}
00000234 <[^>]*> e8740300 ?	ldmda	r4!, {r8, r9}\^
00000238 <[^>]*> e8800002 ?	stmia	r0, {r1}
0000023c <[^>]*> 09820038 ?	stmeqib	r2, {r3, r4, r5}
00000240 <[^>]*> e843ffff ?	stmda	r3, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr, pc}\^
00000244 <[^>]*> e92a05ff ?	stmdb	r10!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r10}
00000248 <[^>]*> e8010007 ?	stmda	r1, {r0, r1, r2}
0000024c <[^>]*> e9020018 ?	stmdb	r2, {r3, r4}
00000250 <[^>]*> e8830003 ?	stmia	r3, {r0, r1}
00000254 <[^>]*> e9e40300 ?	stmib	r4!, {r8, r9}\^
00000258 <[^>]*> ef123456 ?	swi	0x00123456
0000025c <[^>]*> 2f000033 ?	swics	0x00000033
00000260 <[^>]*> ebfffffe ?	bl	00000260 <[^>]*>
[		]*260:.*_wombat.*
00000264 <[^>]*> 5bfffffe ?	blpl	00000264 <[^>]*>
[		]*264:.*ARM.*hohum
00000268 <[^>]*> eafffffe ?	b	00000268 <[^>]*>
[		]*268:.*_wibble.*
0000026c <[^>]*> dafffffe ?	ble	0000026c <[^>]*>
[		]*26c:.*testerfunc.*
