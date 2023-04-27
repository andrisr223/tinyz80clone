#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/utils.h"
#include "shell.h"

#pragma std_c99
//bool requires std-c99 or std-sdcc99 or better
#include "stdbool.h"

/*
 * Environment variables
 */
char ENV_PWD[256]; //  = {"/"};
char ENV_PATH[256];
int8_t ENV_RC = 0;

#define ESCS 0x1b

#define X(a, b, c, d, e, f) [a] = {d, e, f},
    const builtin_params_s builtin_params[] =
    {
        BUILTIN_TABLE
    };
#undef X

static char * shell_read_command_line(char * buf, uint32_t buf_size);
static int8_t shell_find_builtin(info_t *info);
static int8_t shell_handler(info_t *info, const char *command);

static void set_info(info_t *info);
static void free_info(info_t *info, int all);

void shell_loop(info_t *info)
{
    char command_line_buf[256];
    char ** arguments;
    uint8_t status = 1;

    // memcpy(ENV_PWD, "/", 2);
    ENV_PWD[0] = '/';
    ENV_PWD[1] = 0;

    while (1)
    {
        // if interactive
        printf("> ");
        char *command = shell_read_command_line(command_line_buf,
            sizeof(command_line_buf));
        // if interactive
        printf("\r\n");
        if (command[0] == 0)
            continue;

        // debug
        // printf("%s\r\n", command);

        info->arg = command;
        shell_handler(info, "");
    }
}

static char * shell_read_command_line(char * buf, uint32_t buf_size)
{
    uint32_t position = 0;
    char c;
    int8_t is_escape = 0;
    int8_t is_csi = 0;

    // echo on
    // TODO do not use echo in interrupt handler, echo here
    echo_off();
    while ((c = (char) getchar()) != EOF)
    {
        if (c == '\n' || c == '\r')
        {
            buf[position] = 0;
            break;
        }

        if (is_escape == 0 && c == ESCS)
        {
            // escape sequence started
            is_escape = 1;
            continue;
        }
        if (is_escape && c == 0x5b)
        {
            is_csi = 1;
            continue;
        }
        is_escape = 0;
        if (is_csi && (c == 'A' || c == 'B' || c == 'C' || c == 'D'))
        {
            continue;
        }
        is_csi = 0;

        if ((c == '\b' || c == 0x7f) && position > 0)
        {
            buf[--position] = 0;
            putchar(ESCS);
            printf("[K");
            continue;
        }
        // printf("%02x,", c);

        buf[position++] = c;
        putchar(c);

        if (position >= buf_size)
        {
            // extension not really possible
            // return buf as is for now
            break;
        }
    }
    // echo off
    echo_off();

    // ltrim
    uint8_t i = 0;
    while (' ' == buf[i] && i < position) i++;
    char *result = buf + i;

    return result;
}

static int8_t shell_handler(info_t *info, const char *command)
{
    // parse command
    set_info(info);

    int8_t rc = shell_find_builtin(info);
    if (rc == -1)
    {
        // TODO find_cmd(info);
        printf("%s: unknown command\r\n", info->arg);
    }
    free_info(info, 0);

    return 0;
}

static int8_t shell_find_builtin(info_t *info)
{
    int8_t built_in_found = -1;

#define X(a, b, c, d, e, f) {a, c, b},
    const builtin_table_t table[] =
    {
        BUILTIN_TABLE
        {0, NULL, NULL}
    };
#undef X

    for (int8_t i = 0; table[i].type; i++)
        if (strcmp(info->argv[0], table[i].type/*, " \t"*/) == 0)
        {
            built_in_found = 0;

            if (0 == context.is_mounted && (i > CMD_MOUNT))
            {
                /*
                 * This check is necessary as executing disk related commands
                 * (e.g. ls) without mount will lock the Ch376 chip and
                 * only power cycling seems to help after that
                 */
                printf("%s: SD card not mounted\r\n", info->argv[0]);
                ENV_RC = -1;
                break;
            }

            if ((info->argc - 1) > builtin_params[table[i].id].argc_max
                || (info->argc - 1) < builtin_params[table[i].id].argc_min)
            {
                printf("%s: bad argument\r\n", info->argv[0]);
                ENV_RC = -1;
                break;
            }

            resetContext(&context,
                ch376s_cmd_callback,
                ch376s_sequence_status_callback,
                ch376s_sequence_status_callback,
                NULL);

            info->cmd_id = table[i].id;

            ENV_RC = table[i].func(info);

            info->status = 0;
            info->flags = 0;

            break;
        }
    return (built_in_found);
}

static void set_info(info_t *info)
{
    int8_t i = 0, j;

    if (info->arg)
    {
        // TODO flag finding and tokenizing could be joined
        info->flags = 0;

        info->argv = strtow(info->arg, " \t");
        if (!info->argv)
        {
            // a single command
            info->argv = malloc(sizeof(char *) * 2);
            if (info->argv)
            {
                info->argv[0] = strdup(info->arg);
                info->argv[1] = NULL;
            }
        }
        for (i = 0; info->argv && info->argv[i]; i++)
            ;
        info->argc = i;

        // remove flags from the argument list
        // this part is common for all (built in) commands
        if (info->argc > 1)
        {
            char **argv = malloc((1 + info->argc) * sizeof(char *));
            int8_t k = 0;
            for (i = 0; info->argv && info->argv[i]; i++)
                if ('-' == info->argv[i][0])
                {
                    for (j = 1; info->argv[i][j] != 0 && j < 6 /* total option count in enum */; j++)
                    {
                        if (0 == info->argv[i][j])
                        {
                            // only minus sign for flags, invalid
                            if (j == 1)
                            {
                                printf("%s: invalid option: %s\r\n",
                                    info->argv[0], info->argv[i]);
                            }
                            break;
                        }

                        switch (info->argv[i][j])
                        {
                        case 'a': info->flags |= OPT_a; break;
                        case 'l': info->flags |= OPT_l; break;
                        case 'r': info->flags |= OPT_r; break;
                        case 'f': info->flags |= OPT_f; break;
                        case 'p': info->flags |= OPT_p; break;
                        case 'c': info->flags |= OPT_c; break;
                        default:
                            printf("%s: unknown option: %c\r\n", info->argv[0],
                                info->argv[i][j]);
                            break;
                        }
                    }
                    free(info->argv[i]);
                }
                else
                    argv[k++] = info->argv[i];
            argv[k] = NULL;
            free(info->argv);
            info->argv = argv;
        }

        for (i = 0; info->argv && info->argv[i]; i++)
            ;
        info->argc = i;

        // replace_alias(info);
        replace_vars(info);
    }
}

static void free_info(info_t *info, int all)
{
    (void)all;

    ffree(info->argv);
    info->argv = NULL;
    info->path = NULL;
}
