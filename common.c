#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int isNum(char *string)
{
    int x = 0;
    int len = strlen(string);

    while (x < len)
    {
        if (!isdigit(*(string + x)))
            return 1;

        ++x;
    }

    return 0;
}

int parsePort(char *string)
{
    if (isNum(string) != 0)
    {
        return -1;
    }
    int n = atoi(string);
    if (n < 1 || n > 65535)
    {
        return -1;
    }
    return n;
}