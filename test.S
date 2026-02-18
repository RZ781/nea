.thumb
.global syscall
.global _start
.type syscall, %function
.type syscall, %function

syscall:
	push {r7, lr}
	mov r7, r3
	svc #0
	pop {r7, pc}

_start:
	bl main
	mov r0, #0
	mov r3, #1
	bl syscall
