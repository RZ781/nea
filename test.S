.thumb
.global syscall
.global raise
.type syscall, %function
.type raise, %function

raise:
	push {r4, lr}
	mov r4, r0
	movs r7, #20
	svc #0
	mov r1, r4
	movs r7, #37
	svc #0
	pop {r4, pc}

syscall:
	push {r7, lr}
	mov r7, r3
	svc #0
	pop {r7, pc}
