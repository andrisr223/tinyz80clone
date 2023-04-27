#include <stdio.h>
#pragma std_c99
//bool requires std-c99 or std-sdcc99 or better
#include "stdbool.h"

#include "utils/utils.h"
#include "commdef.h"

#define DEBUG 0
#if (DEBUG)
#define debug_print(fmt, ...) \
    do { if (DEBUG) printf(fmt, __VA_ARGS__); } while (0)
#else
#define debug_print(fmt, ...)
#endif /* DEBUG */

const uint8_t reset_sequence[] = {
    SEQ_ITEM_TAG, 1, CMD_RESET_ALL,
    SEQ_SLEEP_TAG,0,
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    SEQ_NCDJ_TAG, 2, ANSW_USB_INT_DISK_READ, 0xff,
    SEQ_ITEM_TAG, 1, CMD_RD_USB_DATA0};

const uint8_t status_sequence[] = {
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS};

const uint8_t check_mount_sequence[] = {
    SEQ_ITEM_TAG, 1, CMD_DISK_CONNECT,
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    SEQ_ECBK_TAG, 0};

const uint8_t mount_sequence[] = {
    SEQ_ITEM_TAG, 2, CMD_SET_USB_MODE, MODE_HOST_0,
    SEQ_SLEEP_TAG,0,
    SEQ_NEEXT_TAG,1, ANSW_RET_SUCCESS,              // exit, if mode failed
    // SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    // SEQ_NEEXT_TAG,1, ANSW_USB_INT_SUCCESS,          // exit, if status failed
    SEQ_ITEM_TAG, 2, CMD_SET_USB_MODE, MODE_HOST_SD,
    SEQ_SLEEP_TAG,0,
    SEQ_NEEXT_TAG,1, ANSW_RET_SUCCESS,              // exit, if SD card not available
    // SEQ_NCDJ_TAG, 2, ANSW_RET_SUCCESS, 0xff,        // exit, if SD card not available
    // SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    // SEQ_NEEXT_TAG,1, ANSW_USB_INT_SUCCESS,          // exit, if status failed
    SEQ_ITEM_TAG, 1, CMD_DISK_MOUNT,
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    SEQ_NEEXT_TAG,1, ANSW_USB_INT_SUCCESS,          // exit, if status failed
    SEQ_ITEM_TAG, 1, CMD_RD_USB_DATA0,
    SEQ_SCBK_TAG, 0,                                // handle response (store disk id)
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    SEQ_SCBK_TAG, 0};

const uint8_t unmount_sequence[] = {
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    // SEQ_ITEM_TAG, 1, CMD_RD_USB_DATA0,
    // SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    SEQ_ITEM_TAG, 2, CMD_SET_USB_MODE, MODE_HOST_0,
    SEQ_SLEEP_TAG,0,
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    SEQ_SCBK_TAG, 0};

const uint8_t disk_info_sequence[] = {
    SEQ_ITEM_TAG, 1, CMD_DISK_QUERY,
    SEQ_ITEM_TAG, 1, CMD_RD_USB_DATA0,
    SEQ_SCBK_TAG, 0,                                // handle response (print disk info)
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    SEQ_ECBK_TAG, 0};


/*
 * TODO use separate CMD_RD_USB_DATA0 for file case and close the file afterwards
 */
uint8_t list_dir_sequence[] = {
    // SEQ_ITEM_TAG, 1 + 8 + 3 + 1 + 2, CMD_SET_FILE_NAME, '*', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    SEQ_ITEM_TAG, 1 + 2, CMD_SET_FILE_NAME, '*', 0,
    SEQ_ITEM_TAG, 1, CMD_FILE_OPEN,
    SEQ_CNDJ_TAG, 2, ANSW_ERR_OPEN_DIR, 0,          // jump to the beginning, retry with * as name
    SEQ_CNDJ_TAG, 2, ANSW_USB_INT_DISK_READ, 20,    // jump to CMD_RD_USB_DATA0
    // should skip CMD_RD_USB_DATA0 if response != ANSW_USB_INT_DISK_READ
    SEQ_ITEM_TAG, 2, CMD_DIR_INFO_READ, 0xff,       // directory info for currently opened file, enum will not work
    SEQ_ITEM_TAG, 1, CMD_RD_USB_DATA0,
    SEQ_SCBK_TAG, 0,                                // handle response (print file name)
    SEQ_ITEM_TAG, 1, CMD_FILE_ENUM_GO,
    SEQ_CNDJ_TAG, 2, ANSW_USB_INT_DISK_READ, 20,    // return to CMD_RD_USB_DATA0
    // SEQ_ITEM_TAG, 2, CMD_FILE_CLOSE, 0,
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS};

uint8_t create_file_sequence[] = {
    SEQ_ITEM_TAG, 1 + 8 + 3 + 1 + 2, CMD_SET_FILE_NAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    SEQ_ITEM_TAG, 1, CMD_FILE_CREATE,
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    SEQ_SCBK_TAG, 0,                                // check result TODO
    SEQ_ITEM_TAG, 2, CMD_DIR_INFO_READ, 0xff,       // directory info for currently opened file
    // should skip CMD_RD_USB_DATA0 if response != ANSW_USB_INT_DISK_READ
    SEQ_ITEM_TAG, 1, CMD_RD_USB_DATA0,
    SEQ_ITEM_TAG, 1 + 2 + 32, CMD_WR_OFS_DATA, 0, sizeof(file_info_t), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    SEQ_ITEM_TAG, 1, CMD_DIR_INFO_SAVE,             // update file info
    SEQ_ITEM_TAG, 2, CMD_FILE_CLOSE, 1,
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS};

uint8_t delete_file_sequence[] = {
    // close an open file to avoid accidental deletion
    SEQ_ITEM_TAG, 2, CMD_FILE_CLOSE, 0,
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    // set the name of the file to delete
    SEQ_ITEM_TAG, 1 + 8 + 3 + 1 + 2, CMD_SET_FILE_NAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    SEQ_ITEM_TAG, 1, CMD_FILE_ERASE,                // returns ANSW_USB_INT_SUCCESS on success
    // SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    SEQ_SCBK_TAG, 0};

uint8_t open_file_sequence[] = {
    SEQ_ITEM_TAG, 1 + 8 + 3 + 1 + 2, CMD_SET_FILE_NAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    SEQ_ITEM_TAG, 1, CMD_FILE_OPEN,
    SEQ_SCBK_TAG, 0,
    SEQ_NEEXT_TAG,1, ANSW_USB_INT_SUCCESS,          // exit, if not file open
    SEQ_CNDJ_TAG, 2, ANSW_ERR_MISS_FILE, 0xff,      // jump to the end
    SEQ_ITEM_TAG, 2, CMD_DIR_INFO_READ, 0xff,       // directory info for currently opened file
    SEQ_SCBK_TAG, 0,
    // should skip CMD_RD_USB_DATA0 if response != ANSW_USB_INT_DISK_READ
    SEQ_ITEM_TAG, 1, CMD_RD_USB_DATA0,
    SEQ_SCBK_TAG, 0};

uint8_t open_dirinfo_file_sequence[] = {
    SEQ_ITEM_TAG, 1 + 8 + 3 + 1 + 2, CMD_SET_FILE_NAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    SEQ_ITEM_TAG, 1, CMD_FILE_OPEN,
    SEQ_CNDJ_TAG, 2, ANSW_ERR_MISS_FILE, 0xff,      // jump to the end
    // should skip CMD_RD_USB_DATA0 if response != ANSW_USB_INT_DISK_READ
    // SEQ_ITEM_TAG, 1, CMD_RD_USB_DATA0,
    // SEQ_SCBK_TAG, 0,
    SEQ_ITEM_TAG, 2, CMD_DIR_INFO_READ, 0xff,       // directory info for currently opened file
    SEQ_SCBK_TAG, 0,
    // should skip CMD_RD_USB_DATA0 if response != ANSW_USB_INT_DISK_READ
    SEQ_ITEM_TAG, 1, CMD_RD_USB_DATA0,
    SEQ_SCBK_TAG, 0};

uint8_t read_file_sequence[] = {
    SEQ_ITEM_TAG, 3, CMD_BYTE_READ, 0x40, 0x00,     // request to read 64 bytes
    SEQ_CNDJ_TAG, 2, ANSW_USB_INT_SUCCESS, 0xff,    // EOF, exit
    SEQ_CNDJ_TAG, 2, ANSW_ERR_FILE_CLOSE, 0xff,     // attempted op on a closed file
    // if response == ANSW_USB_INT_DISK_READ proceed to read
    SEQ_NCDJ_TAG, 2, ANSW_USB_INT_DISK_READ, 0xff,  // jump to end, if !ANSW_USB_INT_DISK_READ
    SEQ_ITEM_TAG, 1, CMD_RD_USB_DATA0,              // actually read the data
    SEQ_SCBK_TAG, 0,                                // handle the result
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    SEQ_CNDJ_TAG, 2, ANSW_USB_INT_DISK_READ, 0      // jump to CMD_BYTE_READ, continue reading
    };

const uint8_t close_file_sequence[] = {
    SEQ_ITEM_TAG, 2, CMD_FILE_CLOSE, 0,
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    // for some reason it is necessary to open / after CMD_FILE_CLOSE
    // otherwise CMD_RD_USB_DATA0 will continue to read from
    // the previously opened file
    SEQ_ITEM_TAG, 1 + 2, CMD_SET_FILE_NAME, '/', 0,
    SEQ_ITEM_TAG, 1, CMD_FILE_OPEN};

const uint8_t close_file_sequence_[] = {
    SEQ_ITEM_TAG, 2, CMD_FILE_CLOSE, 0};

const uint8_t dir_info_sequence[] = {
    SEQ_ITEM_TAG, 2, CMD_DIR_INFO_READ, 0xff,       // directory info for currently opened file
    SEQ_ITEM_TAG, 1, CMD_RD_USB_DATA0,
    SEQ_SCBK_TAG, 0};

uint8_t create_dir_sequence[] = {
    SEQ_ITEM_TAG, 1 + 8 + 3 + 1 + 2, CMD_SET_FILE_NAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    SEQ_ITEM_TAG, 1, CMD_DIR_CREATE,
    SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    SEQ_SCBK_TAG, 0};

uint8_t change_dir_sequence[] = {
    SEQ_ITEM_TAG, 1 + 8 + 3 + 1 + 2, CMD_SET_FILE_NAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    SEQ_ITEM_TAG, 1, CMD_FILE_OPEN,
    SEQ_SCBK_TAG, 0,                                //
    // SEQ_NEEXT_TAG,1, ANSW_ERR_OPEN_DIR,             // exit, if not directory open
    SEQ_ITEM_TAG, 2, CMD_DIR_INFO_READ, 0xff,       // directory info for currently opened file
    // should skip CMD_RD_USB_DATA0 if response != ANSW_USB_INT_DISK_READ
    SEQ_ITEM_TAG, 1, CMD_RD_USB_DATA0,
    SEQ_SCBK_TAG, 0,                                // handle response (update pwd)
    // SEQ_ITEM_TAG, 1, CMD_GET_STATUS,
    // SEQ_SCBK_TAG, 0
    };

// #if (DEBUG)
description_map_t map_answer_desc[] = {
    {ANSW_RET_SUCCESS, "Success"},
    {ANSW_USB_INT_SUCCESS, "Success"},
    /* New USB device connected */
    {ANSW_USB_INT_CONNECT, "Connected"},
    /* USB device unplugged! */
    {ANSW_USB_INT_DISCONNECT, "Disconnected"},
    /* Device is ready */
    {ANSW_USB_INT_USB_READY, "Ready"},
    /* Disk read operation */
    {ANSW_USB_INT_DISK_READ, "Continue with read"},
    /* Disk write operation */
    {ANSW_USB_INT_DISK_WRITE, "Continue with write"},
    /* Operation failure */
    {ANSW_RET_ABORT, "Abort"},
    /* USB storage device error */
    {ANSW_USB_INT_DISK_ERR, "Disk error"},
    /* Buffer overflow */
    {ANSW_USB_INT_BUF_OVER, "Buffer overflow"},
    /* Tried to open a directory with FILE_OPEN */
    {ANSW_ERR_OPEN_DIR, "Opened directory"},
    /* File not found */
    {ANSW_ERR_MISS_FILE, "File not found"},
    /* File or directory with given name already exists */
    {ANSW_ERR_FOUND_NAME, "File already exists"},
    /* Disk disconnected */
    {ANSW_ERR_DISK_DISCON, "Disk not ready"},
    /* Sector size is not 512 bytes */
    {ANSW_ERR_LARGE_SECTOR, "Sector size mismatch"},
    /* Invalid partition type, reformat drive */
    {ANSW_ERR_TYPE_ERROR, "Type error"},
    /* Partition not formatted */
    {ANSW_ERR_BPB_ERROR, "Partition not formatted"},
    /* Disk full */
    {ANSW_ERR_DISK_FULL, "Disk full"},
    /* Directory full */
    {ANSW_ERR_FDT_OVER, "Directory full"},
    /* Attempted operation on closed file */
    {ANSW_ERR_FILE_CLOSE, "File closed"},
    {0, ""}};

const char * const answerMapLookup(uint8_t code)
{
    static const char *placeholder = "";
    const char *description = placeholder;
    for (int8_t i = 0; i < sizeof(map_answer_desc)/sizeof(map_answer_desc[0]); i++)
    {
        if (map_answer_desc[i].code == code)
        {
            description = map_answer_desc[i].description;
            break;
        }
    }
    return description;
}
// #endif /* DEBUG */

static int8_t write(uint8_t const* buf, uint8_t length) __naked
{
    // (void)buf;
    // (void)length;
    // if (buf == (uint8_t const*)0 || length == 0)
    //     return;

    /*
     * https://gist.github.com/Konamiman/af5645b9998c802753023cf1be8a2970
     */
    __asm
        ld      iy,#2
        add     iy,sp                   ; bypass the return address of the function 

        ld      l,(iy)                  ; buf (low) into l
        ld      h,1(iy)                 ; buf (high) into h
        ld      b,2(iy)                 ; length into b

        call    sio_b_write

        ld      l,b                     ; return value, should be 0
        ret
    __endasm;
}

static int8_t sendCommand(ch376s_context_t *context, uint8_t cmd, const uint8_t *data, uint8_t length)
{
    static uint8_t command_buffer[4] = {0x57, 0xAB, 0, 0};

    debug_print("Command: %x, data: %x, data length: %d\r\n", cmd, data, length);

    context->last_command = cmd;
    context->command_active = 1;
    // CMD_SET_FILE_NAME does not generate any response, don't wait for it
    if (cmd == CMD_SET_FILE_NAME
        || cmd == CMD_FILE_CREATE
        || cmd == CMD_FILE_CLOSE
        || cmd == CMD_RESET_ALL
        || cmd == CMD_WR_OFS_DATA
        /*|| cmd == CMD_FILE_ERASE*/)
    {
        context->command_active = 0;
        // TODO clear last response as well?
        context->last_data = 0;
    }

    if (cmd == CMD_DISK_MOUNT)
    {
        context->disk_id_buffer[0] = 0;
        context->is_mounted = 0;
    }

    // case for CMD_RD_USB_DATA0, which is expected to return data length and then data
    // case for CMD_GET_FILE_SIZE, which is expected to return four bytes
    if (CMD_RD_USB_DATA0 == cmd || CMD_GET_FILE_SIZE == cmd)
    {
        context->response_length = 0;
        context->response_buffer_position = 0;
    }

    command_buffer[2] = cmd;
    int8_t rc = write(command_buffer, 3);
    if (0 == rc && data && length > 0)
        rc = write(data, length);

    return rc;
}

int8_t processCommandSequence(ch376s_context_t *context)
{
    if (context->command_sequence == 0 || context->command_sequence_length == 0)
    {
        debug_print("%s: empty sequence\r\n", __func__);
        return -1;
    }
    if (context->command_sequence_position >= context->command_sequence_length)
    {
        debug_print("%s: finished\r\n", __func__);
        return -126;
    }

    uint8_t tag = *(context->command_sequence + context->command_sequence_position);
    uint8_t len = *(context->command_sequence + context->command_sequence_position + 1);
    if ((context->command_sequence_position + len + 2) > context->command_sequence_length /* || len < 0 */)
    {
        debug_print("%s: invalid length: %d for tag %x @ position %d\r\n",
            __func__, len, tag, context->command_sequence_position);
        return -1;
    }

    debug_print("%s: %p, sequence: %p, position: %d, tag: %x, len: %d\r\n",
        __func__, context, context->command_sequence,
        context->command_sequence_position, tag, len);

    int8_t result = 0;
    uint8_t cmd = 0;
    const uint8_t *code = 0;

    uint8_t *data = 0;
    uint8_t data_len = 0;
    uint8_t data_buf[256]; // = {0};

    switch (tag)
    {
        case SEQ_ITEM_TAG:
            cmd = *(context->command_sequence + context->command_sequence_position + 2);

            if (len > 1)
            {
                data = context->command_sequence + context->command_sequence_position + 3;
                data_len = len - 1;
            }

            context->command_sequence_position = context->command_sequence_position + 2 + len;

            if (context->on_command_execute_callback)
            {
                // copy default command data to the buffer
                // callback may modify it
                if (data)
                    _memcpy(data_buf, data, data_len);
                data = data_buf;
                context->on_command_execute_callback(context, cmd,
                    data, &data_len, 256/*sizeof(data_buf)/sizeof(data_buf[0])*/);
            }
            result = sendCommand(context, cmd, data, data_len);
            break;
        case SEQ_JUMP_TAG:
            // destination position in the sequence is in the one data byte
            data = context->command_sequence + context->command_sequence_position + 2;
            context->command_sequence_position = *data;
            // special case to exit
            if (0xff == context->command_sequence_position)
                context->command_sequence_position = context->command_sequence_length;
            break;
        case SEQ_CNDJ_TAG:
        case SEQ_NCDJ_TAG:
            // if last_data matches condition byte,
            // destination position in the sequence is in the one data byte
            code = context->command_sequence + context->command_sequence_position + 2;
            data = context->command_sequence + context->command_sequence_position + 3;
            // debug_print("processCommandSequence: conditional jump: code %x, data, %x\r\n",
            //     *code, *data);
            if ((tag == SEQ_CNDJ_TAG && (*code == context->last_data))
                || (tag == SEQ_NCDJ_TAG && (*code != context->last_data)))
            {
                context->command_sequence_position = *data;
                // special case to exit
                if (0xff == *data)
                    context->command_sequence_position = context->command_sequence_length;
            }
            else
                context->command_sequence_position = context->command_sequence_position + 2 + 1 + 1;
            break;
        case SEQ_EEXT_TAG:
        case SEQ_NEEXT_TAG:
            // if last_data matches condition byte,
            // destination position is sequence end
            // result code is -1
            code = context->command_sequence + context->command_sequence_position + 2;
            if ((tag == SEQ_EEXT_TAG && (*code == context->last_data))
                || (tag == SEQ_NEEXT_TAG && (*code != context->last_data)))
            {
                debug_print("%s: got: %x, expected: %x: command: %02x @ %d\r\n",
                    __func__, context->last_data, *code, context->last_command,
                    context->command_sequence_position);

                context->command_sequence_position = context->command_sequence_length;
                result = -1;
            }
            else
                context->command_sequence_position = context->command_sequence_position + 2 + len;
            break;
        case SEQ_SCBK_TAG:
            context->command_sequence_position = context->command_sequence_position + 2 + len;
            if (context->on_sequence_status_callback)
                result = context->on_sequence_status_callback(context);
            break;
        case SEQ_ECBK_TAG:
            context->command_sequence_position = context->command_sequence_position + 2 + len;
            if (context->on_sequence_error_callback)
                result = context->on_sequence_error_callback(context);
            break;
        // case SEQ_EXIT_TAG:
        //     break;
        case SEQ_SLEEP_TAG:
            context->command_sequence_position = context->command_sequence_position + 2 + len;
            __asm
                ld      d,0xff
                call    sleep
            __endasm;
            break;        
        default:
            debug_print("%s: unknown tag %x in sequence %p @ %d\r\n",
                __func__, tag, context->command_sequence,
                context->command_sequence_position);
            result = -1;
    }

    return result;
}

int8_t startCommandSequence(ch376s_context_t *context,
    uint8_t *sequence, uint8_t sequence_length)
{
    context->last_command = 0;
    context->last_data = 0;
    context->last_error = 0;

    context->command_sequence = sequence;
    context->command_sequence_length = sequence_length;
    context->command_sequence_position = 0;

    return processCommandSequence(context);
}

void resetContext(ch376s_context_t *context,
    ch376s_cmd_callback_t command_callback,
    ch376s_cmd_callback_t status_callback,
    ch376s_cmd_callback_t error_callback,
    ch376s_cmd_callback_t completed_callback)
{
    context->last_command = 0;
    context->last_data = 0;
    context->last_error = 0;
    context->interrupted = 0;
    context->command_active = 0;
    context->command_sequence = 0;
    context->user_data = NULL;

    context->command_callback = command_callback;
    context->on_sequence_status_callback = status_callback;
    context->on_sequence_error_callback = error_callback;
    context->on_sequence_completed_callback = completed_callback;
    context->on_command_execute_callback = NULL;
}

void discardCommandSequence(ch376s_context_t *context)
{
    context->command_sequence = 0;
    context->command_sequence_length = 0;
    context->command_sequence_position = 0;
}

int8_t runCommandSequence(ch376s_context_t *context, ch376s_command_t command)
{
    int8_t rc = command(context);
    if (rc != 0)
    {
        debug_print("%s: Could not start command sequence\r\n", __func__);
        return rc;
    }

    bool finished = false;
    while (!finished)
    {
        if (!context->command_sequence)
        {
            debug_print("%s: Command sequence absent, exit\r\n", __func__);
            finished = true;
            break;
        }

        uint8_t counter = 0;
        while (context->command_active)
        {
            if (counter > 20)
            {
                debug_print("%s: Command %x still active: "
                    "last_data: %x, error: %x, interrupted: %x, counter: %d, "
                    "consider timeout\r\n", __func__,
                    context->last_command, context->last_data,
                        /*error_sio_b*/0, /*context.interrupted*/0, counter);
                discardCommandSequence(context);
                finished = true;
                break;
            }
            counter++;
            sleep();
        }

        if (!context->command_active)
        {
            debug_print("         %x, response: %x (%s)\r\n", context->last_command,
                context->last_data, answerMapLookup(context->last_data));
            rc = processCommandSequence(context);
            if (rc != 0)
            {
                discardCommandSequence(context);
                finished = true;
                // sequence is just exhausted, no error
                if (rc == -126)
                    rc = 0;
            }
        }
    }

    return rc;
}

int8_t fetchVersion(ch376s_context_t *context)
{
    (void)context;

    if (context == 0 || context->command_active)
        return -1;

    return sendCommand(context, CMD_GET_IC_VER, 0, 0);
}

// CMD_SET_USB_MODE data 03: SD host, manage SD cards
int8_t fetchStatus(ch376s_context_t *context)
{
    return sendCommand(context, CMD_GET_STATUS, 0, 0);
}

int8_t fetchDiskInfo(ch376s_context_t *context)
{
    return sendCommand(context, CMD_DISK_QUERY, 0, 0);
}

int8_t fetchInterruptData(ch376s_context_t *context)
{
    return sendCommand(context, CMD_RD_USB_DATA0, 0, 0);
}

int8_t setSDCardMode(ch376s_context_t *context)
{
    // 03: SD host, manage SD cards
    uint8_t mode[] = {MODE_HOST_SD};
    return sendCommand(context, CMD_SET_USB_MODE, mode, 1);
}

#define START_COMMAND(sequence) \
    return startCommandSequence(context, \
        sequence, \
        sizeof(sequence)/sizeof(sequence[0]));

int8_t isMounted(ch376s_context_t *context)
{
    START_COMMAND(check_mount_sequence)
}

int8_t mount(ch376s_context_t *context)
{
    START_COMMAND(mount_sequence)
}

int8_t diskInfo(ch376s_context_t *context)
{
    START_COMMAND(disk_info_sequence)
}

int8_t unmount(ch376s_context_t *context)
{
    START_COMMAND(unmount_sequence)
}

int8_t listDir(ch376s_context_t *context)
{
    START_COMMAND(list_dir_sequence)
}

int8_t makeDir(ch376s_context_t *context)
{
    START_COMMAND(create_dir_sequence)
}

int8_t changeDir(ch376s_context_t *context)
{
    START_COMMAND(change_dir_sequence)
}

int8_t open(ch376s_context_t *context)
{
    START_COMMAND(open_file_sequence)
}

int8_t openWithDirInfo(ch376s_context_t *context)
{
    START_COMMAND(open_dirinfo_file_sequence)
}

int8_t read(ch376s_context_t *context)
{
    START_COMMAND(read_file_sequence)
}

int8_t close(ch376s_context_t *context)
{
    START_COMMAND(close_file_sequence)
}

int8_t touch(ch376s_context_t *context)
{
    START_COMMAND(create_file_sequence)
}

int8_t delete(ch376s_context_t *context)
{
    START_COMMAND(delete_file_sequence)
}

int8_t status(ch376s_context_t *context)
{
    START_COMMAND(status_sequence)
}

// does not help with hard lock ups
int8_t reset(ch376s_context_t *context)
{
    // START_COMMAND(reset_sequence)
    sendCommand(context, CMD_RESET_ALL, 0, 0);
    sleep();
    sleep();
    sleep();
    sendCommand(context, CMD_GET_STATUS, 0, 0);
    return 0;
}

#undef START_COMMAND

// unused
int8_t ch376s_cmd_callback(ch376s_context_t *context)
{
    (void)context;

    return 0;
}

int8_t ch376s_sequence_status_callback(ch376s_context_t *context)
{
    // generic status callback
    if (context == NULL)
        return 0;

    if (CMD_RD_USB_DATA0 == context->last_command)
    {
        uint8_t *dst = 0;
        disk_info_t disk_info;
        file_info_t file_info;

        if (context->response_length == sizeof(disk_info_t))
            dst = (uint8_t *)&disk_info;
        else if (context->response_length == sizeof(file_info_t))
            dst = (uint8_t *)&file_info;
        else if (context->response_length == (sizeof(context->disk_id_buffer)/context->disk_id_buffer[0]))
            dst = (uint8_t *)context->disk_id_buffer;

        if (dst)
        {
            // memcpy(dst, context->response_buffer, context->response_length);
            uint8_t i = 0;
            for (i = 0; i < context->response_length; i++)
            {
                printf("%02x", context->response_buffer[i]);
                dst[i] = context->response_buffer[i];
            }
            puts("\r\n");

            if (context->response_length == sizeof(disk_info_t))
                printf("Disk info: total sectors: %lu, free: %lu, fat type: %x\r\n",
                    disk_info.totalSector,
                    disk_info.freeSector,
                    disk_info.diskFat);
            if (context->response_length == sizeof(file_info_t))
                printf("%.11s size %lu\r\n", file_info.name, file_info.size);
        }
        else
        {
            uint8_t i = 0;
            for (i = 0; i < context->response_length; i++)
                printf("%02x", context->response_buffer[i]);
            puts("\r\n");
        }
    }
    else
    {
        printf("Status: last cmd: %x, last_data: %x (%s)\r\n",
            context->last_command, context->last_data, answerMapLookup(context->last_data));
    }

    return 0;
}
