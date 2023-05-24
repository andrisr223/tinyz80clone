#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <string.h>
#include <sdcc-lib.h>

#include "ch376s/commdef.h"
#include "utils/utils.h"
#include "shell/shell.h"

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
volatile char buf_rx_a[RX_BUFSIZE];
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
    __asm__(" di\n exx\n ex af,af'\n");
    __asm
        call    A_RTS_OFF               ; disable RTS
        in      a,(SIO_DA)              ; read RX character into A
        ld      (_rx_a),a
    __endasm;
    // TODO do not echo special characters: up, left, right, backspace, tab...
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
}

void rst_rx_a() __naked
{
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

    info_t info[] = { INFO_INIT };
    clear();
    shell_loop(info);
}
