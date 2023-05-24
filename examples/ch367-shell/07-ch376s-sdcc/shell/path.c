#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utils/utils.h"
#include "shell.h"

char *shell_sanitize_path(const char *pwd, const char *path)
{
    if (path == NULL)
        return NULL;
    if (path[0] == '/' && path[1] == '0')
        return strdup(path);

    uint16_t len = strlen(path);
    if (path[0] != '/' && pwd)
    {
        // include pwd in the result
        uint8_t pwd_len = strlen(pwd) + /* for / separator */ 1;
        // TODO check for overflow - 
        if (pwd_len > (0xff - len)) // overflow, 254(?) byte path supported
            return NULL;
        len += pwd_len; // strlen(pwd) + /* for / separator */ 1;
    }

    char *result = malloc(len + /* for terminating zero */ 1);
    if (result == NULL)
        return NULL;
    result[0] = 0;

    uint8_t last_dir_position[128];
    int8_t last_dir_index = -1;

    int8_t last_dir = -1;
    int8_t position = 0;

    uint8_t i = 0;
    uint8_t j = 0;

    char *paths[] = {pwd, path, NULL};
    if (path[0] == '/' || pwd == NULL)
        j = 1;

    for (; paths[j] != NULL; j++)
    {
        path = paths[j];
        if (path[0] == '/')
        {
            result[0] = '/';
            result[1] = 0;
            position = 1;
        }

        // TODO probably can be done in-place
        char **items = strtow(path, " /");
        for (i = 0; items && items[i]; ++i)
        {
            if (items[i][0] == '.')
            {
                if (items[i][1] == 0)
                    continue;
                if (items[i][1] == '.')
                {
                    if (items[i][2] == 0)
                    {
                        // erase last dir from result, if a directory already added
                        if (last_dir_index > -1)
                        {
                            // printf("             erasing\n");
                            position = last_dir_position[last_dir_index--];
                            result[position] = 0;
                        }
                        continue;
                    }
                }
            }
            uint8_t len = strlen(items[i]);
            _memcpy(result + position, items[i], len);
            last_dir = position;
            last_dir_position[++last_dir_index] = position;
            result[position + len] = '/';
            position += len + 1;
        }
        ffree(items);
    }
    if (i > 0 && position > 1)
        result[position - 1] = 0;

    return result;
}

char *_shell_normalize_file_name(const file_info_t *file_info, char * const buf, uint8_t length)
{
    if (length < 8 + 3 + 1 + 1)
        return NULL;

    int8_t position = 0;
    // normalize file name in dot format
    // 'A       TXT' becomes 'A.TXT'
    for (int8_t i = 0; i < 8 + 3; i++)
    {
        if (' ' == file_info->name[i])
        {
            buf[position] = '.';
            continue;
        }
        // TODO for ABCDEFGHTXT
        // if (i == 7 && nodot)
        // {
        //     buf[position++] = '.';
        // }
        buf[position++] = file_info->name[i];
    }
    buf[position] = 0;

    return buf;
}

void _shell_file_info_print(const file_info_t *file_info, uint8_t flags)
{
    // skip hidden files if not requested with OPT_a
    if ((file_info->fattr & CH376_ATTR_HIDDEN) && !(flags & OPT_a))
        goto exit_;
    // do not show . and .. TODO this makes .<name> hidden without an attribute
    if (file_info->name[0] == '.'
            // && (file_info->fattr & CH376_ATTR_DIRECTORY)
            && !(flags & OPT_a))
        goto exit_;

    // long listing
    if (flags & OPT_l)
    {
        uint16_t year = file_info->modDate;
        year = year >> 9;
        year += 1980;
        uint16_t month = file_info->modDate;
        month = month << 7;
        month = month >> 12;
        uint16_t day = file_info->modDate;
        day = day << 11;
        day = day >> 11;

        printf("%c%c%c%c%c%c\t%lu\t%u.%u.%u\t",
            (file_info->fattr & CH376_ATTR_DIRECTORY) ? 'd' : '-',
            (file_info->fattr & CH376_ATTR_ARCHIVE) ? 'a' : '-',
            (file_info->fattr & CH376_ATTR_HIDDEN) ? 'h' : '-',
            (file_info->fattr & CH376_ATTR_SYSTEM) ? 's' : '-',
            (file_info->fattr & CH376_ATTR_READ_ONLY) ? 'r' : '-',
            (file_info->fattr & CH376_ATTR_VOLUME_ID) ? 'v' : '-',
            file_info->size,
            year,
            month,
            day);
    }
    // light blue colour for directories
    if (file_info->fattr & CH376_ATTR_DIRECTORY)
        printf("\033[1;34m");
    // green for txt, bas files
    else if (strncmp("TXT", &file_info->name[8], 3) == 0
        || strncmp("BAS", &file_info->name[8], 3) == 0)
        printf("\033[0;32m");
    // \033[0m clears any formatting
    printf("%.11s\033[0m\r\n", file_info->name);
exit_:
    return;
}

int8_t _shell_set_date(file_info_t *file_info, uint16_t year, uint16_t month, uint16_t day)
{
    uint16_t date = file_info->modDate;
    // TODO read the date from the real time clock, if present
    // set year to current year at least
    year = year - 1980;
    year = year << 9;

    // uint16_t month = date << 7;
    // month = month >> 12;
    // uint16_t month = 5;
    month = month << 5;

    // uint16_t day = date << 11;
    // day = day >> 11;
    // uint16_t day = 7;

    file_info->modDate = year + month + day;

    return 0;
}

void _shell_update_pwd(const file_info_t *file_info)
{
    // printf("%s\r\n", __func__);
    uint8_t len = strlen(ENV_PWD);
    char file_name[14];
    int8_t p = 0;
    // normalize file name in dot format
    // 'A       TXT' becomes 'A.TXT'
    for (int8_t i = 0; i < 8 + 3; i++)
    {
        if (' ' == file_info->name[i])
        {
            file_name[p] = '.';
            continue;
        }
        file_name[p++] = file_info->name[i];
    }
    file_name[p] = 0;
    uint8_t len1 = strlen(file_name);

    if (file_name[0] == '/')
    {
        // len = strlen(file_name);
        _memcpy(ENV_PWD, file_name, len1);
        ENV_PWD[len1] = 0;
    }
    else
    {
        if ('/' != ENV_PWD[len - 1])
            ENV_PWD[len++] = '/';
        _memcpy(ENV_PWD + len, file_name, len1);
        ENV_PWD[len + len1] = 0;
        len += len1;
        // TODO rtrim ENV_PWD from space and /
        if (ENV_PWD[len - 1] == '/')
            ENV_PWD[len - 1] = 0;
    }
}
