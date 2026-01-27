.syntax unified
.thumb

.global _start

.section .data
msg:
    .ascii "Hello, World!\n"
msg_len = . - msg

.section .text
_start:
    /* write(1, msg, msg_len) */
    movs r0, #1          /* file descriptor: stdout */
    ldr  r1, =msg        /* pointer to message */
    ldr  r2, =msg_len    /* message length */
    ldr  r7, =4          /* syscall number for sys_write */
    swi 0                 /* make syscall */

    /* exit(0) */
    movs r0, #0          /* exit code */
    ldr  r7, =1          /* syscall number for sys_exit */
    swi 0                 /* make syscall */
