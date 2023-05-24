#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utils/utils.h"
#include "ch376s/commdef.h"

#include "shell.h"

#define USE_UBASIC

#ifdef USE_UBASIC
#include "ubasic/ubasic.h"
#endif /* USE_UBASIC */

typedef struct chdir_context_s
{
    enum_builtin_id_t id;
    char *path;
    uint8_t position;
    uint8_t flags;
} chdir_context_t;

// TODO read/write contexts with path might be file handle
// to implement regular open/close/read/write/seek api.
typedef struct read_context_s
{
    uint32_t cursor_position;
    uint16_t sector_counter;
    uint8_t flags;
} read_context_t;

typedef struct write_context_s
{
    uint32_t data_length;
    uint32_t data_position;
    uint32_t cursor_position;
    uint16_t sector_counter;
    uint8_t *data;
    uint8_t flags;
} write_context_t;

#ifdef USE_UBASIC
typedef struct exec_buffer_s
{
    char *buf;
    uint16_t pos;
    uint16_t length;
} exec_buffer_t;
#endif /* USE_UBASIC */

const char *FMT_TOO_MANY_ARGS = "%s: too many arguments\r\n";
const char *FMT_INVALID_ARGS = "%s: invalid arguments\r\n";
const char *FMT_NO_SUCH_FILE_DIR = "%s: %s: no such file or directory\r\n";
const char *FMT_FILE_ALREADY_EXISTS = "%s: %s: file already exists\r\n";
const char *FMT_IS_DIRECTORY = "%s: %s: is a directory\r\n";

static int8_t _shell_cd_helper(ch376s_context_t *ctx, uint8_t cmd, uint8_t *data, uint8_t *data_length, uint8_t data_length_max);
static int8_t _shell_cd_status(ch376s_context_t *ctx);

static int8_t _shell_rm_status(ch376s_context_t *ctx);

static int8_t _shell_cat_status(ch376s_context_t *ctx);

static int8_t _shell_write_helper(ch376s_context_t *ctx, uint8_t cmd, uint8_t *data, uint8_t *data_length, uint8_t data_length_max);
static int8_t _shell_write_status(ch376s_context_t *ctx);

#ifdef USE_UBASIC
static int8_t _shell_run_status(ch376s_context_t *ctx);
#endif /* USE_UBASIC */

int8_t shell_usage(info_t *info)
{
    (void)info;

    printf("Tiny Z80 shell v0.1\r\n"
        "Available commands:\r\n"
        "\techo\t\t\r\n"
        "\tpwd\t\tshow current working directory\r\n"
        "\thelp\t\tshow this help message\r\n"
        "\tmount\t\tmount the SD card\r\n"
        "\tumount\t\tunmount the SD card\r\n"
        "\treset\t\treset the CH376S chip\r\n"
        "\tstatus\t\tread interrupt status of the CH376S chip\r\n"
        "\tls\t\tlist directory\r\n"
        "\tcd\t\tchange directory\r\n"
        "\tmkdir\t\tmake a directory\r\n"
        "\ttouch\t\tcreate an empty file\r\n"
        "\ttype\t\tprint file contents\r\n"
        "\trm\t\tremove file/directory\r\n"
        "\tdf\t\tshow disk status info\r\n"
        "\trand\t\tprint a one byte random number\r\n"
#ifdef USE_UBASIC
        "\trun\t\texecute a BASIC script\r\n"
#endif /* USE_UBASIC */
        "\tdate\t\tprint current date\r\n"
        );
    return 0;
}

int8_t shell_echo(info_t *info)
{
    (void)info;

    for (int8_t i = 1; i < info->argc; ++i)
        printf("%s\r\n", info->argv[i]);
    return 0;
}

int8_t shell_rand(info_t *info)
{
    (void)info;

    uint8_t result = magic_number();
    printf("%02x\r\n", result);
    return 0;
}

int8_t shell_date(info_t *info)
{
    (void)info;

    printf("%04d.%02d.%02d %02d:%02d:%02d\r\n", 2023, 5, 7, 12, 0, 0);
    return 0;
}

int8_t shell_reset(info_t *info)
{
    info->status = runCommandSequence(&context, reset);

    return info->status;
}

int8_t shell_status(info_t *info)
{
    info->status = runCommandSequence(&context, status);

    return info->status;
}

int8_t shell_mount(info_t *info)
{
    (void)info;

    info->status = runCommandSequence(&context, mount);

    // TODO this may be better handled in commdef.c
    if (info->status == 0)
        context.is_mounted = 1;

    return info->status;
}

int8_t shell_unmount(info_t *info)
{
    info->status = runCommandSequence(&context, unmount);

    ENV_PWD[0] = '/';
    ENV_PWD[1] = 0;

    // TODO this may be better handled in commdef.c
    context.is_mounted = 0;

    return info->status;
}

int8_t shell_df(info_t *info)
{
    info->status = runCommandSequence(&context, diskInfo);
    return info->status;
}

int8_t shell_pwd(info_t *info)
{
    (void)info;

    printf("%s\r\n", ENV_PWD);
    return 0;
}

int8_t shell_cd_pwd()
{
    chdir_context_t chdir_ctx;
    chdir_ctx.id = CMD_CD;
    chdir_ctx.flags = 0;
    chdir_ctx.position = 0;
    chdir_ctx.path = ENV_PWD;

    context.user_data = &chdir_ctx;
    context.on_command_execute_callback = _shell_cd_helper;
    context.on_sequence_status_callback = NULL;

    int8_t status = 0;
    while (status == 0 && chdir_ctx.position < strlen(chdir_ctx.path))
        status = runCommandSequence(&context, changeDir);

    resetContext(&context,
        ch376s_cmd_callback,
        ch376s_sequence_status_callback,
        ch376s_sequence_status_callback,
        NULL);

    return 0;
}

/*
 * To reduce code size (avoid repetitive checks, use common code between commands)
 * main file operations squeezed into following (large) functions.
 */
int8_t shell_combined(info_t *info)
{
    // argument count, flags validity should be checked earlier in shell.c

    chdir_context_t chdir_ctx;
    chdir_ctx.id = info->cmd_id;
    chdir_ctx.flags = info->flags;
    chdir_ctx.position = 0;
    chdir_ctx.path = NULL;

    if (CMD_LS == info->cmd_id && info->argc == 1)
    {
        shell_cd_pwd();

        context.user_data = &chdir_ctx;
        context.on_command_execute_callback = _shell_cd_helper;
        context.on_sequence_status_callback = _shell_cd_status;
        info->status = runCommandSequence(&context, listDir);
        goto exit_;
    }

    for (uint8_t i = 1; i < info->argc && info->status == 0; ++i)
    {
        // TODO can allow NULL, then the special case above would be solved
        if (NULL == info->argv[i])
            continue;

        context.on_command_execute_callback = _shell_cd_helper;
        context.on_sequence_status_callback = _shell_cd_status;

        context.user_data = &chdir_ctx;
        chdir_ctx.id = CMD_CD;

        chdir_ctx.position = 0;
        // printf("%s: pwd: %s\r\n", __func__, ENV_PWD);
        // printf("%s: param: %s\r\n", __func__, info->argv[i]);
        chdir_ctx.path = shell_sanitize_path(ENV_PWD, info->argv[i]);
        // printf("%s: sanitized path: %s\r\n", __func__, chdir_ctx.path);

        uint8_t path_length;
        if (info->cmd_id == CMD_CD)
            path_length = strlen(chdir_ctx.path);
        else
        {
            char *separator = strrchr(chdir_ctx.path, '/');
            if (separator == NULL || separator == chdir_ctx.path)
                // skip cd, proceed with the requested command
                goto command_;

            // assume and cd to path sans the last element
            // last element will be passed to the info->cmd_id command
            path_length = separator - chdir_ctx.path;

            // do not update PWD for ls, mkdir, type, rm, touch
            chdir_ctx.flags |= OPT_u;
        }

        while (info->status == 0 && chdir_ctx.position < path_length)
            info->status = runCommandSequence(&context, changeDir);

        if (info->status != 0)
        {
            printf(FMT_NO_SUCH_FILE_DIR, info->argv[0], info->argv[i]);
            goto close_;
        }
        else
        {
            if ((chdir_ctx.flags & OPT_u) == 0)
            {
                uint32_t len = strlen(chdir_ctx.path);
                _memcpy(ENV_PWD, chdir_ctx.path, len);
                ENV_PWD[len] = 0;
            }

        }

        // change directory command finished
        if (info->cmd_id == CMD_CD)
            goto close_;

command_:
        if (chdir_ctx.position >= strlen(chdir_ctx.path))
        {
            // path is exhausted, reset
            if (chdir_ctx.path)
                free(chdir_ctx.path);
            chdir_ctx.path = NULL;
            chdir_ctx.position = 0;
        }

        chdir_ctx.id = info->cmd_id;
        context.on_command_execute_callback = _shell_cd_helper;

        ch376s_command_t command;
        switch (info->cmd_id)
        {
        case CMD_RM:
            command = delete;
            break;
        case CMD_TOUCH:
            context.on_sequence_status_callback = NULL;
            command = touch;
            break;
        case CMD_LS:
            command = listDir;
            break;
        case CMD_MKDIR:
            command = makeDir;
            break;
        // TODO cleaner file ops
        case CMD_CAT:
            context.on_sequence_status_callback = _shell_cd_status;
            info->status = runCommandSequence(&context, open);
            // TODO file should be closed regardless of the status later
            if (0 == info->status)
            {
                read_context_t read_ctx;
                read_ctx.cursor_position = 0;
                read_ctx.sector_counter = 0;
                read_ctx.flags = info->flags;

                context.user_data = &read_ctx;
                context.on_command_execute_callback = NULL;
                context.on_sequence_status_callback = _shell_cat_status;
                info->status = runCommandSequence(&context, read);
            }

            // context.user_data = &chdir_ctx;
            // context.on_command_execute_callback = NULL;
            // context.on_sequence_status_callback = NULL;
            command = close;
            break;
        case CMD_RUN:
        {
            context.on_sequence_status_callback = _shell_cd_status;
            info->status = runCommandSequence(&context, open);
            // TODO file should be closed regardless of the status change later
            if (0 == info->status)
            {
                file_info_t *file_info = (file_info_t *)context.response_buffer;
                if (file_info->size == 0)
                {
                    command = close;
                    break;
                }

                exec_buffer_t exec_buf;
                exec_buf.pos = 0;
                exec_buf.length = file_info->size + 1;
                exec_buf.buf = malloc(exec_buf.length);
                if (exec_buf.buf == NULL)
                {
                    info->status = -1;
                    printf("%s: not enough memory\r\n", info->argv[0]);
                }
                else
                {
                    // memset
                    for (uint32_t i = 0; i < exec_buf.length; ++i)
                        exec_buf.buf[i] = 0;
                    context.user_data = &exec_buf;
                    context.on_command_execute_callback = NULL;
                    context.on_sequence_status_callback = _shell_run_status;
                    info->status = runCommandSequence(&context, read);

                    ubasic_init(exec_buf.buf);
                    do
                    {
                        ubasic_run();
                    } while(!ubasic_finished());
                    free(exec_buf.buf);
                }
            }

            // context.user_data = &chdir_ctx;
            // context.on_command_execute_callback = NULL;
            // context.on_sequence_status_callback = NULL;
            command = close;
        }
            break;
        case CMD_APPEND:
        {
            // exhaust params to avoid outer loop over arguments
            i = info->argc - 1;
            context.on_sequence_status_callback = _shell_cd_status;
            info->status = runCommandSequence(&context, open/*OrCreate*/);
            // TODO file should be closed regardless of the status change later
            if (0 == info->status)
            {
                write_context_t write_ctx;
                write_ctx.cursor_position = 0;
                write_ctx.sector_counter = 0;
                write_ctx.flags = info->flags;

                write_ctx.data = info->argv[i];
                write_ctx.data_length = strlen(info->argv[i]);
                write_ctx.data_position = 0;

                context.user_data = &write_ctx;
                context.on_command_execute_callback = _shell_write_helper;
                context.on_sequence_status_callback = _shell_write_status;
                info->status = runCommandSequence(&context, write);
            }

            // context.user_data = &chdir_ctx;
            // context.on_command_execute_callback = NULL;
            // context.on_sequence_status_callback = NULL;
            command = close;
        }
            break;
        default:
            // TODO bad command
            info->status = -1;
            break;
        }

        if (command && close != command)
        {
            int8_t rc = runCommandSequence(&context, command);
            info->status = 0 == info->status ? rc : info->status;
        }

close_:
        if (chdir_ctx.path)
            free(chdir_ctx.path);

        context.user_data = NULL;
        context.on_command_execute_callback = NULL;
        context.on_sequence_status_callback = NULL;
        runCommandSequence(&context, close);
    }

exit_:
    return info->status;
}

static int8_t _shell_cd_helper(ch376s_context_t *ctx, uint8_t cmd, uint8_t *data, uint8_t *data_length, uint8_t data_length_max)
{
    static const char OFFSET = 'a' - 'A';

    if (ctx == NULL)
        return -1;

    chdir_context_t *chdir_ctx = ctx->user_data;

    if (chdir_ctx == NULL)
        return -1;

    if (cmd == CMD_SET_FILE_NAME)
    {
        char * path = chdir_ctx->path;
        uint8_t position = chdir_ctx->position;
        if (chdir_ctx->position >= strlen(chdir_ctx->path))
        {
            path = NULL;
            position = 0;
        }

        switch (chdir_ctx->id)
        {
        case CMD_LS:
            if (path == NULL) path = "*";
            goto default_;
        case CMD_CD:
            if (path == NULL) path = "/";
        default:
        {
default_:
            if (path == NULL)
                return -1;

            // copy part of the path to the command buffer
            uint8_t i = 0;
            while (i < CH376_FILE_NAME_LEN_MAX)
            {
                // skip a / in the middle
                if (i > 0 && path[position + i] == '/')
                {
                    chdir_ctx->position = position + i + 1;
                    break;
                }

                uint8_t c = path[position + i];
                // convert to upper case
                data[i] = (c >= 'a' && c <= 'z') ? c - OFFSET : c;
                if (c == 0)
                {
                    chdir_ctx->position = position + i;
                    break;
                }
                ++i;
            }
            data[i] = 0;
            *data_length = i + 1;
            // printf("%s: %s, %d (%d), position: %d (%d)\r\n",
            //     __func__,
            //     data, *data_length, data_length_max,
            //     chdir_ctx->position, strlen(path));
        }
        break;
        }
    }
    else if (cmd == CMD_WR_OFS_DATA)
    {
        switch (chdir_ctx->id)
        {
        case CMD_TOUCH:
        {
            // here we rely on CMD_DIR_INFO_READ content still being available
            file_info_t *file_info = (file_info_t *)ctx->response_buffer;
            _shell_set_date(file_info, 2023, 5, 7);

            _memcpy((void *)(data + 2), (void *)file_info, sizeof(file_info_t));
            data[0] = 0;
            data[1] = sizeof(file_info_t);
            *data_length = 2 + sizeof(file_info_t);
        }
        break;
        default: break;
        }

    }
    return 0;
}

static int8_t _shell_cd_status(ch376s_context_t *ctx)
{
    if (ctx == NULL)
        return -1;

    chdir_context_t *chdir_ctx = ctx->user_data;

    if (chdir_ctx == NULL)
        return -1;

    int8_t rc = 0;

    if (ctx->last_command == CMD_FILE_OPEN)
    {
        switch (chdir_ctx->id)
        {
        case CMD_CD:
            if (ctx->last_data == ANSW_ERR_MISS_FILE || ctx->last_data != ANSW_ERR_OPEN_DIR)
            {
                // TODO close file
                rc = -1;
            }
            break;
        default: break;
        }
    }
    else if (ctx->last_command == CMD_RD_USB_DATA0)
    {
        if (ctx->response_length == sizeof(file_info_t))
        {
            file_info_t *file_info = (file_info_t *)ctx->response_buffer;

            switch (chdir_ctx->id)
            {
            case CMD_LS:
                _shell_file_info_print(file_info, chdir_ctx->flags);
                break;
            case CMD_CD:
                // handle CMD_DIR_INFO_READ result
                if (!(file_info->fattr & CH376_ATTR_DIRECTORY))
                {
                    // TODO not a directory
                    // TODO close file
                    rc = -1;
                }

                // special case for / which does not get a proper file info
                // (returned buffer is all zeros but has a length)
                if ((chdir_ctx->flags & OPT_u) == 0)
                    if (chdir_ctx->path[0] == '/' && chdir_ctx->path[1] == 0)
                    {
                        // update PWD
                        // ENV_PWD[0] = '/';
                        // ENV_PWD[1] = 0;
                        rc = 0;
                    }

                break;
            case CMD_CAT:
            case CMD_RUN:
                if (file_info->fattr & CH376_ATTR_DIRECTORY)
                {
                    // TODO is a directory
                    // TODO close directory
                    rc = -1;
                }
                break;
            default:
                break;
            }
        }
    }
    else if (ctx->last_command == CMD_FILE_ERASE)
    {
        if (CMD_RM == chdir_ctx->id)
        {
            if (ctx->last_data != ANSW_USB_INT_SUCCESS)
            {
                // printf("%s: %s\r\n", "rm", answerMapLookup(ctx->last_data));
                rc = -1;
            }
        }
    }

    return rc;
}

static int8_t _shell_cat_status(ch376s_context_t *ctx)
{
    if (ctx == NULL)
        return 0;

    read_context_t *read_ctx = ctx->user_data;

    if (read_ctx == NULL)
        return -1;

    if (ctx->last_command == CMD_RD_USB_DATA0)
    {
        if (read_ctx->flags & OPT_c)
        {
            // TODO save status between calls
            // (use a different struct than chdir_context_t)
            char ascii[17];
            ascii[16] = '\0';
            unsigned char *buf = ctx->response_buffer;
            for (uint8_t i = 0; i < ctx->response_length; ++i)
            {
                printf("%02X ", buf[i]);
                if (buf[i] >= ' ' && buf[i] <= '~')
                    ascii[i % 16] = buf[i];
                else
                    ascii[i % 16] = '.';
                if ((i + 1) % 8 == 0 || i + 1 == ctx->response_length)
                {
                    printf(" ");
                    if ((i + 1) % 16 == 0)
                        printf("%s\r\n", ascii);
                    else if (i + 1 == ctx->response_length)
                    {
                        ascii[(i + 1) % 16] = '\0';
                        if ((i + 1) % 16 <= 8)
                            printf(" ");
                        for (uint8_t j = (i + 1) % 16; j < 16; ++j)
                            printf("   ");
                        printf("%s\r\n", ascii);
                    }
                }
            }
        }
        else
        {
            uint8_t last_char = 0;
            for (uint8_t i = 0; i < ctx->response_length; ++i)
            {
                if ('\n' == ctx->response_buffer[i] && last_char != '\r')
                    putchar('\r');
                putchar(ctx->response_buffer[i]);
                last_char = ctx->response_buffer[i];
            }
        }

        /*
         * Sector check should not be a responsibility of a shell command.
         * This could be solved by always requesting SECTOR_SIZE bytes for read
         * if memory is not a constraint.
         */
        read_ctx->sector_counter += ctx->response_length;
        if (read_ctx->sector_counter >= SECTOR_SIZE)
        {
            ctx->last_data = ANSW_ERR_NEXT_SECTOR;
            read_ctx->sector_counter = 0;
        }
    }

    return 0;
}

static int8_t _shell_write_helper(ch376s_context_t *ctx, uint8_t cmd, uint8_t *data, uint8_t *data_length, uint8_t data_length_max)
{
    if (ctx == NULL)
        return -1;

    write_context_t *info = ctx->user_data;

    if (info == NULL)
        return -1;

    if (cmd == CMD_BYTE_LOCATE)
    {
        // in case of append, seek to the end of the file
        // if info->id == CMD_APPEND
        data[0] = 0xff;
        data[1] = 0xff;
        data[2] = 0xff;
        data[3] = 0xff;
        *data_length = 4;
    }
    else if (cmd == CMD_BYTE_WRITE)
    {
        uint32_t remaining_length = info->data_length - info->data_position;
        // write 64 bytes at a time (255 may be allowed maximum)
        data[0] = remaining_length > 64 ? 64 : (uint8_t) remaining_length;
        data[1] = 0;
        *data_length = 2;
    }
    // else if (cmd == CMD_WR_REQ_DATA)
    // {
    // }
    else if (cmd == __CMD_RAW_WRITE)
    {
        // ctx->last_data here holds the answer to CMD_WR_REQ_DATA
        // which is the length we are allowed to write at this point
        _memcpy(data, info->data + info->data_position, ctx->last_data);
        *data_length = ctx->last_data;
        info->data_position += ctx->last_data;
    }

    return 0;
}

static int8_t _shell_write_status(ch376s_context_t *ctx)
{
    if (ctx == NULL)
        return -1;

    write_context_t *info = ctx->user_data;

    if (info == NULL)
        return -1;

    // if (ctx->last_command == CMD_WR_REQ_DATA)
    // {
    //     if (ctx->last_data == 0 && info->data_length > 0)
    //         return -1;
    // }

    return 0;
}

#ifdef USE_UBASIC
static int8_t _shell_run_status(ch376s_context_t *ctx)
{
    if (ctx == NULL)
        return -1;

    exec_buffer_t *info = ctx->user_data;

    if (info == NULL)
        return -1;

    if (ctx->last_command == CMD_RD_USB_DATA0)
    {
        if (info->pos + ctx->response_length < info->length)
        {
            _memcpy(info->buf + info->pos,  ctx->response_buffer, ctx->response_length);
            info->pos += ctx->response_length;
        }
        else
            return -1;
    }

    return 0;
}
#endif /* USE_UBASIC */