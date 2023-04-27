#include "shell.h"

#include <stdlib.h>
#include <string.h>

/**
 * replace_vars - replaces vars in the tokenized string
 * @info: the parameter struct
 *
 * Return: 1 if replaced, 0 otherwise
 */
int8_t replace_vars(info_t *info)
{
    int8_t i = 0;

    for (i = 0; info->argv[i]; i++)
    {
        if (info->argv[i][0] != '$' || !info->argv[i][1])
            continue;

        if (0 == strcmp(info->argv[i], "$?"))
        {
            replace_string(&(info->argv[i]),
                strdup(convert_number(ENV_RC, 10, 0)));
            continue;
        }
/*
        if (0 == strcmp(info->argv[i], "$$"))
        {
            replace_string(&(info->argv[i]),
                strdup(convert_number(getpid(), 10, 0)));
            continue;
        }
 */
        replace_string(&info->argv[i], strdup(""));

    }
    return (0);
}

/**
 * replace_string - replaces string
 * @old: address of old string
 * @new: new string
 *
 * Return: 1 if replaced, 0 otherwise
 */
int8_t replace_string(char **old, char *new)
{
    free(*old);
    *old = new;
    return (1);
}


/**
 * convert_number - converter function, a clone of itoa
 * @num: number
 * @base: base
 * @flags: argument flags
 *
 * Return: string
 */
char *convert_number(long int num, int base, int flags)
{
	static char *array;
	static char buffer[50];
	char sign = 0;
	char *ptr;
	unsigned long n = num;

	if (!(flags & CONVERT_UNSIGNED) && num < 0)
	{
		n = -num;
		sign = '-';

	}
	array = flags & CONVERT_LOWERCASE ? "0123456789abcdef" : "0123456789ABCDEF";
	ptr = &buffer[49];
	*ptr = '\0';

	do	{
		*--ptr = array[n % base];
		n /= base;
	} while (n != 0);

	if (sign)
		*--ptr = sign;
	return (ptr);
}