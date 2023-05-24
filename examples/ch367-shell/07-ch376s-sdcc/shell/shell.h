#pragma once

#include <stdint.h>

#include "ch376s/commdef.h"

extern volatile ch376s_context_t context;

extern char ENV_PWD[256];
extern char ENV_PATH[256];
extern int8_t ENV_RC;

enum builtin_id;

typedef struct info_s
{
    char *arg;
    char **argv;
    char *path;
    int8_t argc;

    enum builtin_id cmd_id;

    // int8_t argp; // currently at arg position from argc; TODO this is ugly 
    // int8_t argp; // position in argument from argi
    // int8_t argi; // argv index

    // unsigned int line_count;
    int8_t err_num;
    uint8_t flags;

    // int linecount_flag;
    // char *fname;
    // list_t *env;
    // list_t *history;
    // list_t *alias;
    char **environ;
    int8_t env_changed;
    int8_t status;

    char **cmd_buf; /* pointer to cmd ; chain buffer, for memory management */
    int8_t cmd_buf_type; /* CMD_type ||, &&, ; */
    int8_t readfd;
    int histcount;
} info_t;

#define INFO_INIT \
{NULL, NULL, NULL, 0, 0,/*0,*/ 0, 0, /*0, NULL, NULL, NULL, NULL, */ \
    NULL, 0, 0, NULL, \
    0, 0, 0}

/* for convert_number() */
#define CONVERT_LOWERCASE   1
#define CONVERT_UNSIGNED    2

/**
 * accepted options, shared by all commands
 */
enum
{
    OPT_r = (1 << 0), // recursive
    OPT_a = (1 << 1), // all; for ls - show hidden files
    OPT_f = (1 << 2), // force (ignore error)
    OPT_l = (1 << 3), // for ls - long listing
    OPT_p = (1 << 4), // for mkdir, create path if it does not exist
    OPT_c = (1 << 5), // for cat - hex listing
    OPT_u = (1 << 6), // for all cds - do not update ENV_PWD
};

#define BUILTIN_TABLE \
    X(CMD_HELP, shell_usage, "help", 0, 1, 0) \
    X(CMD_ECHO, shell_echo, "echo", 0, 0x7f, 0) \
    X(CMD_RAND, shell_rand, "rand", 0, 0, 0) \
    X(CMD_RESET, shell_reset, "reset", 0, 0, 0) \
    X(CMD_STATUS, shell_status, "status", 0, 0, 0) \
    X(CMD_DATE, shell_date, "date", 0, 0, 0) \
    X(CMD_MOUNT, shell_mount, "mount", 0, 0, 0) \
    X(CMD_UNMOUNT, shell_unmount, "umount", 0, 0, 0) \
    X(CMD_DF, shell_df, "df", 0, 0, 0) \
    X(CMD_PWD, shell_pwd, "pwd", 0, 0, 0) \
    X(CMD_CD, shell_combined, "cd", 1, 1, 0) \
    X(CMD_MKDIR, shell_combined, "mkdir", 1, 1, OPT_p) \
    X(CMD_TOUCH, shell_combined, "touch", 1, 0x7f, 0) \
    X(CMD_RM, shell_combined, "rm", 1, 0x7f, OPT_r | OPT_f) \
    X(CMD_LS, shell_combined, "ls", 0, 0x7f, OPT_a | OPT_l | OPT_r) \
    X(CMD_CAT, shell_combined, "type", 1, 1, OPT_c) \
    X(CMD_RUN, shell_combined, "run", 1, 1, 0) \
    X(CMD_APPEND, shell_combined, "append", 2, 2, 0)

/**
 * built-in command enum
 */
#define X(a, b, c, d, e, f) a,
typedef enum builtin_id
{
    BUILTIN_TABLE
} enum_builtin_id_t;
#undef X

/**
 * struct builtin_table_s - contains a builtin string, id and related function
 */
typedef struct builtin_table_s
{
    enum_builtin_id_t id;
    const char *type;
    int8_t (*func)(info_t *);
} builtin_table_t;

typedef struct builtin_params_s
{
    uint8_t argc_min;
    uint8_t argc_max;
    uint8_t flag_mask;
} builtin_params_t;

void shell_loop(info_t *info);

char *strdup(const char *str);
char **strtow(char *str, char *d);
void ffree(char **);
int8_t replace_vars(info_t *info);
int8_t replace_string(char **old, char *new);
char *replace_escapes(char *str);
char *convert_number(long int num, int base, int flags);

int8_t shell_cd_pwd();

int8_t shell_usage(info_t *info);
int8_t shell_reset(info_t *info);
int8_t shell_status(info_t *info);
// int8_t shell_cat(info_t *info);
int8_t shell_df(info_t *info);
//int8_t shell_ls(info_t *info);
int8_t shell_unmount(info_t *info);
int8_t shell_mount(info_t *info);
int8_t shell_pwd(info_t *info);
int8_t shell_rand(info_t *info);
int8_t shell_echo(info_t *info);
int8_t shell_cd(info_t *info);
//int8_t shell_mkdir(info_t *info);
//int8_t shell_touch(info_t *info);
//int8_t shell_rm(info_t *info);
int8_t shell_run_basic(info_t *info);
int8_t shell_combined(info_t *info);
int8_t shell_date(info_t *info);

char *shell_sanitize_path(const char *pwd, const char *path);
char *_shell_normalize_file_name(const file_info_t *file_info, char * const buf, uint8_t length);
void _shell_file_info_print(const file_info_t *file_info, uint8_t flags);
int8_t _shell_set_date(file_info_t *file_info, uint16_t year, uint16_t month, uint16_t day);
void _shell_update_pwd(const file_info_t *file_info);

extern const builtin_params_t builtin_params[];
