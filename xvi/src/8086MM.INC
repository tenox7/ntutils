; Copyright (c) 1990,1991,1992 Chris and John Downey
;***
;
; program name:
;	xvi
; function:
;	PD version of UNIX "vi" editor, with extensions.
; module name:
;	8086mm.inc
; module function:
;	8086 assembly language macros, mainly to help make assembly
;	language modules memory model independent.
;
;	This isn't really needed for Xvi (which uses the large memory
;	model), but if we want to provide a portable terminal
;	interface library which can also be used by other
;	applications, it will almost certainly be needed.
; history:
;	STEVIE - ST Editor for VI Enthusiasts, Version 3.10
;	Originally by Tim Thompson (twitch!tjt)
;	Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
;	Heavily modified by Chris & John Downey
;***

;
; Memory model definitions.
;
s	equ	0
m	equ	1
c	equ	2
l	equ	3

;
; Define sizes of code & data pointers.
;
if MEMMODEL eq s
    CPTRSIZE	=	2
    DPTRSIZE	=	2
endif

if MEMMODEL eq m
    CPTRSIZE	=	4
    DPTRSIZE	=	2
endif

if MEMMODEL eq c
    CPTRSIZE	=	2
    DPTRSIZE	=	4
endif

if MEMMODEL eq l
    CPTRSIZE	=	4
    DPTRSIZE	=	4
endif

;
; Useful macros defined according to the memory model.
;

;
; Declare an external function written in C.
;
C_extern	macro	funcname
	if CPTRSIZE eq 4
		extrn	funcname : far
	else
		extrn	funcname : near
	endif
		endm

;
; Return to C caller.
;
C_ret		macro
	if CPTRSIZE eq 4
		retf
	else
		db	0c3h	;; Near return.
	endif
		endm

;
; Call another routine which returns with a C_ret & is in the same
; segment as the caller.
;
Cn_call		macro	func
	if CPTRSIZE eq 4
		push	cs
	endif
		call	func
		endm

;
; Assign a one-word value by dereferencing a pointer.
;
; In C, this would be:
;
;	*memptr = value;
;
; Note that:
;
;	- bx is destroyed. For compact & large memory models, ds is
;	  also destroyed.
;
;	- for small & medium memory models, memptr is a near pointer,
;	  which must be relative to ds.
;
ptrasg		macro	memptr, value
	if DPTRSIZE eq 4
		lds	bx, dword ptr memptr
		assume	ds: nothing
	else
		mov	bx, memptr
	endif
		mov	[bx], value
		endm

;
; Generally useful macros, defined here for convenience.
;

;
; Clear a register to 0.
;
clear		macro	reg
		xor	reg, reg
		endm

;
; Test a register for equality with 0.
;
tst		macro	reg
		or	reg, reg
		endm
