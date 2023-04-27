;
;			Example C project for use with zasm
; 			(c) 2014 - 2017	kio@little-bat.de
;
; NOTE:
; this file is not included in the zasm self test
; because SDCC generates different code every time.
;

.reqcolon

; declare target:
;
#target rom


; define rom and ram size and address:
;
_rom_start::        equ 0
;_rom_end::         equ 0x0500
; _rom_end::          equ 0x51d0 ; 20944
; _rom_end::          equ 0x44f0 ; 17648
_rom_end::          equ 0x6000
; _rom_end::          equ 0x6D00
;_rom_end::          equ 0x77F0
; _rom_end::        equ 0x7fff
_ram_start::        equ 0x8000
_ram_end::          equ 0xffff
_min_heap_size::    equ 0x1000  ; for malloc

;
; Register definitions for PIO, CTC, SIO
; This program is for Tiny Z80 clone with Z84, set Z84 to 1
;
#define Z84 1
#include "../includes/firmware.asm"


; ________________________________________________________________
; Define order of code segments in rom:
; these segments produce code in the output file!
;
#code   _HEADER, _rom_start     ; RST vectors et.al.
#code   _HOME                   ; code that must not be put in a bank switched part of memory.
#code   _CODE                   ; most code and const data go here
#code   _GSINIT                 ; init code: the compiler adds some code here and there as required
                                ; note: the final ret from _GSINIT is at the end of this file.
#code   _INITIALIZER            ; initializer for initialized data in ram
#code   _ROM_PADDING
        defs    _rom_end-$      ; pad rom file up to rom end


; ________________________________________________________________
; Define order of data segments in ram:
; these segments define addresses: no code is stored in the output file!
;
#data   _DATA, _ram_start       ; uninitialized data
#data   _INITIALIZED            ; initialized with data from code segment _INITIALIZER

#data   _HEAP                   ; heap
__sdcc_heap_start::             ; --> sdcc _malloc.c
        ds	_min_heap_size      ; minimum required size for malloc/free
        ds	_ram_end-$-1        ; add all unused memory to the heap
__sdcc_heap_end::               ; --> sdcc _malloc.c
        ds 	1


; ________________________________________________________________
; Declare segments which are referenced by sdcc
; but never (?) used:
;
#data   _DABS,*,0               ; used by sdcc: .area _DABS (ABS): absolute external ram data?
#code   _CABS,*,0               ; used by sdcc: .area _CABS (ABS): ?
#code   _GSFINAL,*,0            ; used by sdcc: .area _GSFINAL: ?
#data   _RSEG,*,0               ; used by kcc:  .area _RSEG (ABS)



; ================================================================
;   Actual Code
;   Rom starts at 0x0000
; ================================================================


;   reset vector
;   RST vectors
;   INT vector (IM 1)
;   NMI vector
;
#code _HEADER
    .globl _rst_rx_a
    .globl _chinchar
    .globl _ch376_get_status
    .globl _irq_56
; RST 0: reset 
    di
    ld      sp,0x0000                   ; Set stack pointer directly above top of memory.
    jp      _GSINIT                     ; Initialize global variables, call main() and exit

; RST 1:
    org 8
rst08:
    jp      TXA
    ; reti

; RST 2:
    org 16  ; $10
    jp      _rst_rx_a                   ; RXA
    ; reti

; RST 3:
    org 24  ; $18
    jp      _chinchar                   ; CKINCHAR
    ;reti

; RST 4:
    org 32
    reti

; RST 5:
    org 40
    reti

; RST 6:
    org 48
    reti

; ________________________________________________________________
; RST 7: maskable interrupt handler in IM 1:
    org 56
    ; jp      _irq_56
    di
    exx
    ex af,af'

    ; ld      hl,MSG_INTERRUPT
    ; call    RAWPRINT

    ; call    _ch376_get_status

    ld      a,057h                      ;
    out     (SIO_DB),a                  ;
    call    TX_EMP_B                    ; wait for outgoing char to be sent

    ; ld      d,#SHORT_SLEEP
    ; call    sleep2

    ld      a,0abh                      ;
    out     (SIO_DB),a                  ;
    call    TX_EMP_B                    ; wait for outgoing char to be sent

    ; ld      d,#SHORT_SLEEP
    ; call    sleep2

    ld      a,22h                       ; GET_STATUS
    out     (SIO_DB),a                  ;
    call    TX_EMP_B                    ; wait for outgoing char to be sent

    exx
    ex af,af'
    ei
    reti

; ________________________________________________________________
; non-maskable interrupt handler:
; must return with RETN

	org	0x66
	rst	0
	;retn

    ds      0x100-($&0xff)  ; align to a 256-byte boundary
vectab:
    .globl	_irq_rx_a
    .globl	_irq_rx_b
vectab_sio:     ; vectab_sio-vectab MUST be a multiple of 8 due to SIO requirements
    defw    irq_tbmt_b
    defw    irq_ext_b
    defw    _irq_rx_b
    ; defw    irq_rx_b
    defw    irq_rxs_b

    defw    irq_tbmt_a
    defw    irq_ext_a
    defw    _irq_rx_a
    defw    irq_rxs_a
vectab_ctc:
    ; dw      irq_ctc_0    ;10
    ; dw      irq_ctc_1    ;12
    ; dw      irq_ctc_2    ;14
    ; dw      irq_ctc_3    ;16

.org     0x0200


; ________________________________________________________________
; globals and statics initialization
; copy flat initializer data: 
;
#code _GSINIT
    ld      bc,_INITIALIZER_len	        ; length of segment _INITIALIZER
    ld      a,b
    or      c
    jr      z,1$+2
    ld      de,_INITIALIZED             ; start of segment _INITIALIZED
    ld      hl,_INITIALIZER             ; start of segment _INITIALIZER
1$: ldir
    ; initialize sio, set up interrupt table
    call    sioa_init
    call    siob_init
    call    siob_relay_init
    ; set mode 2 interrupts & load the vector table address into I
    ld      a,vectab/256                ; A = MSB of the vectab address
    ld      i,a
    im      2                           ; interrupt mode 2
    ei                                  ; enable interrupts


; ________________________________________________________________
; putchar()
; target system dependent:
;
#code _CODE
_putchar::
    ld      hl,2
    add     hl,sp
    ; ld      l,(hl)
    ; ld      a,1
    ld      a,(hl)
    rst     8
    ret

_putchar_rr_dbs::
    ld      a,e
    rst     8
    ret

_getchar::
    ; rst     18h                         ; Check input status
    ; ret     z                           ; No key, go back
    rst     10h                         ; Get the key into a
    ld      l,a
    ret

_echo_on::
    ex      af,af'
    ld      a,1
    ld      (_rx_echo),a
    ex      af,af'
    ret

_echo_off::
    ex      af,af'
    xor     a
    ld      (_rx_echo),a
    ex      af,af'
    ret

; Interrupt handlers
; SIO interrupt handlers
;
irq_tbmt_b:
    di
    out     (SIO_DB),a                  ; echo char to transmitter
    call    TX_EMP                      ; wait for outgoing char to be sent
    ei
    reti

irq_ext_b:
    di
    push    af
    ld      a,'('                       ; backup AF
    out     (SIO_DA),a                  ; echo char to transmitter
    call    TX_EMP                      ; wait for outgoing char to be sent
    pop     af
    ei
    ei
    reti

irq_rx_b:
    ei
    reti

irq_rxs_b:
    di
    ; ex      af,af'
    push    af

    ld      a,00000001b                 ; write into WR0: select RR1
    out     (SIO_CB),a
    in      a,(SIO_CB)                  ; read the error
    ld      (_error_sio_b),a

    ld      a,00000000b                 ; write into WR0: select WR0
    out     (SIO_CB),a
    ld      a,00110000b                 ; error reset
    out     (SIO_CB),a
    ld      a,00000000b                 ; write into WR0: select WR0
    out     (SIO_CB),a
    ld      a,00010000b                 ; reset ext/status interrupts
    out     (SIO_CB),a

    pop     af
    ; ex      af,af'
    ei
    reti

irq_tbmt_a:
    ei
    reti

irq_ext_a:
    ei
    reti

irq_rx_a:
    ei
    reti

irq_rxs_a:
    ei
    reti

; write contents of buffer pointed by hl to the B serial port
; b contains the number of bytes to write
;
sio_b_write:
    push    af
    push    hl
1$:
    ld      a,(hl)
    inc     hl
    out     (SIO_DB),a
    call    TX_EMP_B                    ; wait for outgoing char to be sent
    djnz    1$
    ; call    sleep_sio_b
    pop     hl
    pop     af
    ret

; ________________________________________________________________
; SDCC does not generate a .globl statement for these labels:
; they must be declared before they are used,
; else they are not marked 'used' and #include library won't load them
;
    .globl	__mulint
    .globl	__divsint
    .globl	__divuint
    .globl	__modsint
    .globl	__moduint
    .globl	__muluchar
    .globl	__mulschar
    .globl	__divuchar
    .globl	__divschar
    .globl	__mullong
    .globl	__divulong
    .globl	__divslong
    .globl	__rrulong
    .globl	__rrslong
    .globl	__rlulong
    .globl	__rlslong


CR              equ     0dh
LF              equ     0ah
ESCS            equ     1bh

MSG_CLEAR_SCREEN:
        defm ESCS,"[2J"                 ; Clear screen
        defm ESCS,"[H" ; ESCS,"[38;5;112m"
        defm CR,LF,0
MSG_INTERRUPT:
        defm "Got an interrupt"
        defm CR,LF,0
MSG_NEW_LINE:
        defm CR,LF,0

#include "../includes/sleep.asm"
#include "../includes/sio.asm"

; setup SDCC:
;
; #cpath "sdcc"
#CPATH "/usr/bin/sdcc"
; #cflags $CFLAGS --nostdinc -Iinclude --reserve-regs-iy
; #cflags $CFLAGS --nostdinc --reserve-regs-iy --lib-path /usr/share/sdcc/lib -lz80
; #cflags $CFLAGS -V --reserve-regs-iy -L/usr/share/sdcc/lib -I. -lz80
; #CFLAGS  $CFLAGS --nostdinc -I../sdcc/include   ; add some flags for sdcc
; --reserve-regs-iy
#CFLAGS $CFLAGS -V --opt-code-size -L/usr/share/sdcc/lib -I. -lz80


; ________________________________________________________________
; The Program:
;
#include "main.c"
; For a smaller version of printf:
; #include "printf_simple.c"
#include "ch376s/commdef.c"
#include "shell/shell.c"
#include "shell/tokenizer.c"
#include "shell/realloc.c"
#include "shell/builtin.c"
#include "shell/vars.c"
#include "shell/path.c"
#include "utils/utils.c"
#include "ubasic/ubasic.c"
#include "ubasic/tokenizer.c"
; #include library "library"        ; resolve missing labels from library
#include standard library

; ________________________________________________________________
; calculate length of initializer data:
;
#code _INITIALIZER
_INITIALIZER_len:: equ $ - _INITIALIZER

; ________________________________________________________________
; start system
; after all initialization in _GSINIT is done
;
#code 	_GSINIT
        call    _main       ; execute main()
_exit:: di                  ; system shut down
        halt                ; may resume after NMI
        rst     0           ; then reboot

#end
