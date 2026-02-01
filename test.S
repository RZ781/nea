.thumb
.global syscall
.type syscall, %function

syscall:
	push {r7, lr}
	mov r7, r3
	svc #0
	pop {r7, pc}
