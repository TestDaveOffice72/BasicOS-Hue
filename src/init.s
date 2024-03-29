[BITS 64]

; Our binaries are 4GB onwards.
org 0x100000000;

section .text
global start
start:
; Init program’s own stack
mov rbp, stackend
mov rsp, stackend

; Try printing something
mov rax, msg
mov rcx, 6
int 34


; forks happen
mov rcx, another_app
int 35

mov dx, 03f8h;
mov al, '+';
out dx, al;
mov al, 0x0a;
out dx, al;

; yields happen
int 36;

mov dx, 03f8h;
mov al, '*';
out dx, al;
mov al, 0x0a;
out dx, al;

; yields happen
int 36;

push rax
mov rcx, qword 0x16
int 37
mov rdx, 0x0a40404040404040
mov qword [rax], rdx
mov rcx, rax
mov rdx, 8
int 34

; poweroff machine, we’re done
int 38


; the other app
another_app:
mov rbp, another_app_stackend
mov rsp, another_app_stackend

mov dx, 03f8h;
mov al, '+';
out dx, al;
mov al, 0x0a
out dx, al;

; yields happen
int 36;

mov dx, 03f8h;
mov al, '-';
out dx, al;
mov al, 0x0a
out dx, al;

; draw the flag of the glorious republic of lituhania.
int 39;

; yields happen
b2: int 36;
jmp b2


stack: resb 1024
stackend:

another_app_stack: resb 1024
another_app_stackend:

msg: db "Hello", 0x0a

