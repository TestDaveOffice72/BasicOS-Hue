[BITS 64]

; Our binaries are 4GB onwards.
org 0x100000000;

section .text
global start
start:
// Init program’s own stack
mov rbp, stackend
mov rsp, stackend

// Try printing something
mov rax, msg
mov rcx, 6
int 34

push rdx
push rcx
push rbx
push rax

mov dx, 03f8h;
mov al, '+';
out dx, al;
mov al, '\n';
out dx, al;

b1: hlt
jmp b1
;
; ud2
; int 33

stack: resb 1024
stackend:
msg: db "Hello\n"
