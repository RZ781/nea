.thumb
.global syscall
.type syscall, %function

syscall:
	mov r7, r3
	svc #0
	bx lr
