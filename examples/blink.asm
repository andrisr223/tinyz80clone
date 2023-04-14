;
;
I2C             equ 10000000b ; 80h
LONG_SLEEP      equ 0xff
BLINK_ON        equ 00001000b           ; fourth bit (D3) can be used as I2C status
                                        ; or as a blink led

.org     0x0000

; #if 0
; ; remove cpu clock divide
;         ; load MCR into SCRP
;         ld      a,03h
;         out     (0xEE),a
;         ; read MCR value
;         in      a,(0xEF)
;         ; set clock divide bit so the cpu clock does not get divided
;         or      00010000b
;         out     (0xEF),a
; #endif

        ld      a,#BLINK_ON
main:
        out     (I2C),a

; sleep
        ld      d,#LONG_SLEEP
sleep_loop1:
        ld      b,0xff
sleep_loop2:
        djnz    sleep_loop2
        dec     d
        jp      nz,sleep_loop1

        and     #BLINK_ON               ; check whether the light is on
        jr      nz,blink_off            ; if it is on, switch it off

        ld      a,#BLINK_ON             ; it is off, switch it on
        jr      main

blink_off:
        xor     a

        jr      main

