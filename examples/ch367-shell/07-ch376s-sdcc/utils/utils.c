#include "utils.h"

// #include <stdio.h>

void clear()
{
    __asm
        ld      hl,MSG_CLEAR_SCREEN
        call    RAWPRINT
    __endasm;
}


void remove_clockdivide()
{
    // Z84 specific: remove cpu clock divide
    __asm
        ld      a,03h                   ; load MCR into SCRP
        out     (0xEE),a
        in      a,(0xEF)                ; read the MCR value
        or      00010000b               ; set clock divide bit so the cpu clock does not get divided
        out     (0xEF),a
    __endasm;
}

uint8_t magic_number() __naked
{
    __asm
        in      a,(RNG)
        ld      l,a
        ret
  __endasm;
}

uint16_t rand_()
{
    uint16_t result = magic_number();
    result = (result << 8) + magic_number();
    // printf("%s: %u\r\n", __func__, result);
    return result;
}

void sleep()
{
    __asm
        ld      d,0x0f
        call    sleep
    __endasm;
}

void long_sleep()
{
    __asm
        ld      d,0xff
        call    sleep
        ld      d,0xff
        call    sleep
        ld      d,0xff
        call    sleep
    __endasm;
}

void *_memcpy(void *dst, const void *src, uint16_t n)
{
    unsigned char *s1_ = dst;
    const unsigned char *s2_ = src;
    for (uint16_t i = 0; i < n; ++i)
    {
        // printf("%02x", s2_[i]);
        s1_[i] = s2_[i];
    }

    return dst;
}