; Copyright (c) 1990,1991,1992 Chris and John Downey
.286P

I286_TEXT	segment word public 'CODE'
		db	"@(#)i286.asm	2.1 (Chris & John Downey) 7/29/92"
		db	0
;***
;
; program name:
;	xvi
; function:
;	PD version of UNIX "vi" editor, with extensions.
; module name:
;	i286.asm
; module function:
;	Low-level 80286-specific routines for OS/2 version.
; history:
;	STEVIE - ST Editor for VI Enthusiasts, Version 3.10
;	Originally by Tim Thompson (twitch!tjt)
;	Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
;	Heavily modified by Chris & John Downey
;
;***
		public	_es0
		public	_newstack
;
; void far es0(void);
;
; Set the ES register to 0. Microsoft say this should be done
; before calling the OS/2 DosCreateThread() system call but, as
; far as I can see, they don't provide any C library function or
; compiler directive or any other easy way of doing it. So, if
; you want to write multi-thread code for OS/2, you just have to
; be an assembler hacker.
;
_es0		proc	far
		xor	ax, ax
		mov	es, ax
		ret
_es0		endp

;
; unsigned char far * far newstack (int sub);
;
; This is utterly disgusting, but unavoidable. The fault lies
; with the design of OS/2, & the lack of provision for multiple
; stacks in Microsoft's high-level language products.
;
; We need the provision for multiple stacks because we are
; running multiple threads, but some Microsoft library functions
; are evidently compiled with stack probes, which give spurious
; stack overflow errors if we use multiple stacks. So we just
; use some space at the bottom of our default (48 kb) stack. The
; return value we want here is the top of the new stack, & we
; obtain it simply by subtracting the sub parameter from the
; stack pointer.
;
_newstack	proc	far
		mov	bx, sp
		;
		; Return offset in AX.
		;
		mov	ax, sp
		sub	ax, word ptr ss:[bx + 4]
		;
		; Return stack segment in DX.
		;
		mov	dx, ss
		ret
_newstack	endp
I286_TEXT	ends
		end
