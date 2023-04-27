#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <string.h>
#include <sdcc-lib.h>

#include "ch376s/commdef.h"
#include "utils/utils.h"
#include "shell/shell.h"

#if 0
const char *menu =
    " v Fetch CH376 version\r\n"
    " p Fetch interrupt data\r\n"
    // " f Fetch disk info\r\n"
    " s Get disk status\r\n"
    " n Check mount/device status\r\n"
    " m Mount\r\n"
    " i Get disk info\r\n"
    " l List root directory\r\n"
    " u Unmount\r\n"
    " r Read a random number\r\n"
    " c Redraw the menu\r\n"
    "\r\n"
    " Press a key to continue...\r\n";
#endif

volatile ch376s_context_t context;

volatile uint8_t interrupted;
volatile uint8_t error_sio_b;
volatile disk_info_t disk_info;
volatile file_info_t file_info;

#define RX_BUFSIZE 0x3f
#define RX_FULLSIZE  0x30
#define RX_EMPTYSIZE 0x05

volatile uint8_t rx_echo = 1;
volatile char rx_a = 0;
volatile char buf_rx_a[RX_BUFSIZE]; // = {0};
volatile uint8_t *buf_rx_a_end = buf_rx_a + RX_BUFSIZE;
volatile uint8_t pos_used_rx_a = 0;

volatile uint8_t pos_write_rx_a = 0;
volatile uint8_t pos_read_rx_a = 0;

volatile uint8_t *ptr_write_rx_a = buf_rx_a;
volatile uint8_t *ptr_read_rx_a = buf_rx_a;

volatile char read_a;

// placeholder for channel B input
volatile char rx_b = 0;

#pragma std_c99
//bool requires std-c99 or std-sdcc99 or better
#include "stdbool.h"
bool f;

void irq_rx_a(void) __naked __critical __interrupt
{
#if 1
    __asm__(" di\n exx\n ex af,af'\n");
    __asm
        call    A_RTS_OFF               ; disable RTS
        in      a,(SIO_DA)              ; read RX character into A
        ld      (_rx_a),a
    __endasm;
    // TODO do not echo special characters up left right, backspace, tab...
    if (rx_echo && (rx_a == '\r' || (rx_a >= ' ' && rx_a < 127)))
    {
    __asm
        ld      a,(_rx_a)
        out     (SIO_DA),a              ; echo char to transmitter
        call    TX_EMP                  ; wait for outgoing char to be sent
    __endasm;
        // printf("%x", rx_a);
    }

    buf_rx_a[pos_write_rx_a] = rx_a;
    pos_write_rx_a++;
    pos_used_rx_a++;

    if (pos_write_rx_a >= sizeof(buf_rx_a)/sizeof(buf_rx_a[0]))
        pos_write_rx_a = 0;
    // stop receiving characters when the buffer gets full
    if (pos_used_rx_a >= RX_FULLSIZE)
    {
    __asm
        call    A_RTS_OFF               ; stop receiving further chars        
    __endasm;
    }
    else
    {
    __asm
        call    A_RTS_ON
    __endasm;
    }
    __asm__(" exx\n ex af,af'\n ei\n reti\n");
#else
    __asm__(" di\n exx\n ex af,af'\n");
    __asm
        in      a,(SIO_DA)              ; read RX character into A
        ld      (_rx_a),a
        push    af                      ; store it
        ld      a,(_pos_used_rx_a)      ; load buffer size
        cp      RX_BUFSIZE              ; if buffer is not full
        jr      nz,_not_full                   ; then store the char
        pop     af                      ; else drop it
        jr      exit                      ; and exit
not_full:
        ld      de,(_buf_rx_a_end)
        ld      hl,(_ptr_write_rx_a)    ; buffer is not full, can store the char
        inc     hl                      ; load pointer to find first free cell
        push    hl
        ; ld      a,l                     ; only check low byte because buffer<256
        ; cp      e                       ; check if the pointer is at the last cell
        sbc     hl,de
        add     hl,de
        pop     hl
        jr      nz,not_wrap                   ; if not then continue
        ld      hl,(_buf_rx_a)          ; else load the address of the first cell
not_wrap:
        ld      (_ptr_write_rx_a),hl    ; store the new pointer
        pop     af                      ; then recover the char
        ld      (hl),a                  ; and store it in the appropriate cell
        ld      a,(_pos_used_rx_a)      ; load the size of the serial buffer
        inc     a                       ; increment it
        ld      (_pos_used_rx_a),a      ; and store the new size
        cp      RX_FULLSIZE             ; check if serial buffer is full
        jr      c,exit                    ; exit if buffer is not full
        call    A_RTS_OFF               ; else stop receiving further chars
exit:
        out     (SIO_DA),a              ; echo char to transmitter
        call    TX_EMP                  ; wait for outgoing char to be sent
    __endasm;
    if (rx_echo)
    {
    __asm
        out     (SIO_DA),a              ; echo char to transmitter
        call    TX_EMP                  ; wait for outgoing char to be sent
    __endasm;
    }
    __asm__(" exx\n ex af,af'\n ei\n reti\n");
#endif
}

void rst_rx_a() __naked
{
#if 1
    // block, wait for input, could use chinchar
    while (pos_used_rx_a == 0) {};
    __asm
        di
    __endasm;
        read_a = buf_rx_a[pos_read_rx_a++];
        pos_used_rx_a--;
        if (pos_read_rx_a >= sizeof(buf_rx_a)/sizeof(buf_rx_a[0]))
            pos_read_rx_a = 0;
        // restore reception
        if (pos_used_rx_a <= RX_EMPTYSIZE /* && reception disabled */)
        {
        __asm
            call    A_RTS_ON            ; else re-enable receiving chars
        __endasm;
        }
    __asm
        ld      a,(_read_a)
        ei
        ret
    __endasm;
#else
    __asm
4$:
        ld      a,(_pos_used_rx_a)      ; load the buffer size
        cp      0                       ; check if is 0 (empty)
        jr      z,4$                    ; if it is empty, wait for a char

        di                              ; disable interrupts
        push    hl                      ; store HL
        push    de
        ld      hl,(_ptr_read_rx_a)     ; load pointer to first available char
        ld      de,(_buf_rx_a_end)
        inc     hl                      ; increment it, go to the next char
        ld      a,l                     ; check if the end of the buffer has been reached
        cp      e                       ; only check low byte because buffer<256
        jr      nz,5$                   ; if not, jump straight
        ld      hl,(_buf_rx_a)          ; else reload the starting address of the buffer
5$:
        ld      (_ptr_read_rx_a),hl     ; store new pointer to the next char to read
        ld      a,(_pos_used_rx_a)      ; load buffer size
        dec     a                       ; decrement it
        ld      (_pos_used_rx_a),a      ; and store the new size
        cp      RX_EMPTYSIZE            ; check if serial buffer can be considered empty
        jr      nc,6$                   ; if not empty yet, then exit
        call    A_RTS_ON                ; else re-enable receiving chars
6$:
        ld      a,(hl)                  ; recover the char and return it into A
        pop     de
        pop     hl                      ; recover H
        ei                              ; re-enable interrupts
        ret
    __endasm;
#endif
}

void irq_rx_b(void) __naked /*__critical */ __interrupt
{
    __asm__(" di\n exx\n ex af,af'\n");
    __asm
        ;call    B_RTS_OFF               ; disable RTS
        in      a,(SIO_DB)              ; read RX character into A
        ld      (_rx_b),a
        ; call    B_RTS_ON                ; disable RTS
        ; out     (SIO_DA),a            ; debug, echo output to channel a
    __endasm;
    context.last_data = rx_b;

    // CMD_RD_USB_DATA0 needs fast, minimal handling to avoid Rx overflow issue
    if (CMD_RD_USB_DATA0 == context.last_command)
    {
        // first received byte indicates the length of the data that follows
        if (context.response_length == 0)
        {
            context.response_length = rx_b;
            context.response_buffer_position = 0;
        }
        else
            context.response_buffer[context.response_buffer_position++] = rx_b;

        if (context.response_length <= context.response_buffer_position)
            context.command_active = 0;
    }
    else if (CMD_GET_FILE_SIZE == context.last_command)
    {
        // file size is returned in four bytes without size indication
        if (context.response_length == 0)
        {
            context.response_length = 4;
            context.response_buffer_position = 0;
        }

        context.response_buffer[context.response_buffer_position++] = rx_b;

        if (context.response_length <= context.response_buffer_position)
            context.command_active = 0;
    }
    else
    {
        // for commands with single byte responses callback somewhat works
        // context.command_callback(&context, rx_b);
        context.command_active = 0; //?
        context.last_data = rx_b;
    }
    __asm__(" exx\n ex af,af'\n ei\n reti\n");
}

// DO NOT call this function from C code!
void chinchar() __naked
{
    __asm
        ld      a,(_pos_used_rx_a)
        cp      $00                     ; compare to 0
        ret
    __endasm;
}


void init();

void main()
{
    remove_clockdivide();

    // initialize ch376
    reset(&context);
    context.is_mounted = 0;
    // set up the ch376 context
    resetContext(&context,
        ch376s_cmd_callback,
        ch376s_sequence_status_callback,
        ch376s_sequence_status_callback,
        NULL);

#if 0
    init();
    while (1)
    {
        // putchar(' ');
        loop();
    }
#else
    info_t info[] = { INFO_INIT };
    clear();
    shell_loop(info);
#endif
}

#if 0
void init()
{
    clear();
    puts(menu);
}


void loop()
{
    char cmd;
loop:
    error_sio_b = 0;
    cmd = getchar();
    switch (cmd)
    {
    case 'v': fetchVersion(&context); break;
    case 's': // fetchStatus(&context);
        status(&context);
        break;
    case 'p': fetchInterruptData(&context); break;
    case 'n': isMounted(&context); break;
    case 'm': // setSDCardMode(&context);
        mount(&context); break;
    case 'i':
        disk_info.totalSector = 0;
        disk_info.freeSector = 0;
        disk_info.diskFat = 0;
        getDiskInfo(&context);
        break;
    case 'l':
        context.last_data = 0;
        // listDirectory(&context, "*", 1);
        break;
    case 'u': unmount(&context); break;
    case 'r':
        {
            uint8_t num = magic_number();
            printf("Random number: %x", num);
            printf(", %x\r\n", random);
        }
        goto loop;
    case 'c':
        clear();
        puts(menu);
        goto loop;
    default:
        goto loop;
    }

    bool finished = false;
    while (!finished)
    {
        if (!context.command_sequence)
        {
            printf("Command sequence absent, exit\r\n");
            finished = true;
            // break;
        }
        
        uint8_t counter = 0;
        while (context.command_active)
        {
            if (counter > 10)
            {
                printf("Command %x still active: error: %x, interrupted: %x, counter: %d, consider timeout\r\n",
                    context.last_command, error_sio_b, context.interrupted, counter);
                    discardCommandSequence(&context);
                    finished = true;
                    break;
            }
            counter++;
            sleep();
        }

        if (!context.command_active)
        {
            // printf("Callback loop: command: %x, data: %x\r\n", context.last_command, context.last_data);

            int8_t err = processCommandSequence(&context);
            if (err == -1)
            {
                discardCommandSequence(&context);
                finished = true;
            }
        }
    }
}
#endif