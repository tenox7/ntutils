;
; This function intercepts the keyboard interrupt before BIOS gets it. It
; allows access to almost every combination of key. As such, simul101.asm is
; no longer needed, as this routine has much the same affect.
;
; It works by disabling the BIOS keyboard flags. This fools the BIOS into
; accepting the key. For example, instead of ignoring "c+1", it now accepts
; it, as "1". However, "c+a" becomes "a", not character 1. The real shift
; state is stored here as kbd_shift.
;
; This function was designed to be linked with C object code.  The prototype
; for this function is far so it may be easily used with any memory model.
;
;
; Assembler flags:
;
;      QuickAssembler:   qcl /c simul101.asm
;            MASM 6.0:   ml /c /Cp /Zm simul101.asm
;                TASM:   tasm /Mx /t /z
;
; Editor name:   TDE, the Thomson-Davis Editor.
; Author:        Jason Hood
; Date:          August 20, 2002, version 5.1g
;
; This code is released into the public domain, Jason Hood. You may
; distribute it freely.

kb_data_port    EQU     60h
kb_cntl_port    EQU     61h

bios_data       SEGMENT AT 40h
                ORG     17h
kb_status       DW      ?
                ORG     19h
kb_keypad       DB      ?
bios_data       ENDS


_TEXT   SEGMENT WORD PUBLIC 'CODE'
        ASSUME  cs:_TEXT, ds:_TEXT, es:bios_data
        public  _kbd_install
        public  _kbd_shift
        public  _kbd_menu


_kbd_shift      DB      0, 0    ; the actual shift state
_SHIFT          equ     2       ; bit 1 of the hi-byte, not bit 9 of the word
_CTRL           equ     4
_ALT            equ     8

_kbd_menu       DB      0       ; TRUE if the Menu key was pressed


;
; Prototype this function as far in the C header file so it may be used easily
; with any memory model.  See the last section in tdefunc.h for more info.
;
_kbd_install    PROC    FAR
        jmp     initialize


old_int_9       DW      ?,?     ; space for old interrupt
prefix          DB      0       ; prefix code for extended keys

start:
        push    ax      ; push the registers we use
        push    dx
        push    ds
        push    es

        mov     dx, cs                  ; put code segment in dx
        mov     ds, dx                  ; now transfer code segment to ds

        in      al, kb_data_port        ; let's look at the waiting key

        cmp     al, 0e0h                ; prefix code?
        jne     k1
        mov     prefix, 1
        jmp     regular_int_9

k1:
        mov     ah, _kbd_shift+1        ; the actual modifier state

        cmp     al, 2ah                 ; is it lshift?
        je      shift                   ; yes
        cmp     al, 36h                 ; is it rshift?
        jne     k2
shift:
        cmp     prefix, 0               ; ignore extended shift keys
        jne     fake_bios
        or      ah, _SHIFT              ; indicate shift is pressed
        jmp     short fake_bios
k2:
        cmp     al, 1dh                 ; is it control?
        jne     k3
        or      ah, _CTRL               ; indicate control is pressed
        jmp     short fake_bios
k3:
        cmp     al, 38h                 ; is it alt?
        jne     k4
        or      ah, _ALT                ; indicate alt is pressed
        jmp     short fake_bios
k4:
        cmp     al, 0aah                ; is it lshift released?
        je      no_shift
        cmp     al, 0b6h                ; is it rshift released?
        jne     k5
no_shift:
        cmp     prefix, 0
        jne     fake_bios
        and     ah, not _SHIFT          ; indicate shift is not pressed
        jmp     short fake_bios
k5:
        cmp     al, 09dh                ; is it control released?
        jne     k6
        and     ah, not _CTRL           ; indicate control is not pressed
        jmp     short fake_bios
k6:
        cmp     al, 0b8h                ; is it alt released?
        jne     k7
        and     ah, not _ALT            ; indicate alt is not pressed
k7:
        cmp     al, 05dh                ; is it menu pressed?
        jne     fake_bios
        inc     _kbd_menu               ; indicate it
        jmp     short reg_09

fake_bios:
        mov     dx, 0040h               ; segment of bios data area
        mov     es, dx                  ; put it in es
        mov     dx, WORD PTR es:kb_status  ; get shift status
        and     dx, not 0104h              ; turn the controls off

        ; leave BIOS control on to allow for shift and/or control prtsc
        cmp     al, 37h                 ; prtsc...
        jne     k8
        cmp     prefix, 1               ; ...is a prefixed key
        jne     k8
        test    ah, ah                  ; leave unmodified go through
        jz      set_state
        or      dl, 4                   ; BIOS control flag
        jmp     short set_state
k8:
        ; turn BIOS Alt off to allow Alt+KP0 to enter NUL
        test    ah, _ALT                ; is alt down?
        jz      set_state               ; no
        cmp     al, 52h                 ; KP0 (insert)
        jne     set_state
        cmp     prefix, 0               ; don't want grey insert
        jne     set_state
        cmp     es:kb_keypad, 0         ; nothing has been entered,
        jne     set_state               ;  so return Alt+KP0
        and     dx, not 0208h           ; disable BIOS alt flags

set_state:
        mov     es:kb_status, dx        ; update the fake BIOS status
        mov     _kbd_shift+1, ah        ; and the real status
reg_09:
        mov     prefix, 0               ; ready prefix flag for the next key

regular_int_9:
        pop     es                      ; restore registers
        pop     ds
        pop     dx
        pop     ax
        jmp     DWORD PTR cs:old_int_9  ; no interrupt return - old one does it

; ***********************************************************************
; prototype for _kbd_install is
;
;               void far kbd_install( int )
;
; The formal parameter is available on the stack.  Use the bp register to
; access it.
;
; Passing any non-zero value will make this function grab interrupt 9.
; Pass a zero to this function to restore the old interrupt 9.
;
; If this function were really clever, it would have a "unique" signature.
; Before "installing", it would check to see if it was already installed.
; Similarly, before "uninstalling", this function would check to make sure
; it was installed so it wouldn't uninstall the regular interrupt 9
; handler by accident.  What the hell, live dangerously.
;
; ***********************************************************************

initialize:
        push    bp
        mov     bp, sp

        mov     dx, [bp+6]      ; put the parameter in dx

        push    ds
        ASSUME  es:_TEXT,ds:_TEXT
        mov     ax, cs          ; put cs in ds
        mov     es, ax
        mov     ds, ax

        cmp     dx, 0           ; 'NULL' character unhooks interrupt 9
        je      restore_9       ; any non NULL character grabs interrupt 9
grab_9:
        mov     ax, 3509h       ; get old interrupt 9 location
        int     21h             ; call MSDOS to get it
        mov     WORD PTR old_int_9, bx          ; save old int 9 offset
        mov     WORD PTR old_int_9+2, es        ; save old int 9 segment

        mov     dx, OFFSET start        ; get new offset of int 9
        mov     ax, 2509h               ; use function 25 so int 9 points
        int     21h                     ;  to my routine

        mov     _kbd_shift+1, 0         ; assume no shift keys are pressed
        jmp     SHORT get_out           ; continue with editor

restore_9:
        mov     dx, WORD PTR old_int_9          ; get offset of old int 9
        mov     ax, WORD PTR old_int_9+2        ; get segment of old int 9
        mov     ds, ax                          ;  put segment in ds
        mov     ax, 2509h                       ; restore old int 9
        int     21h

get_out:
        pop     ds              ; clean up
        pop     bp
        retf
_kbd_install    endp
_TEXT   ends
        end
