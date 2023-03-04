#include <stdio.h>
#include <stdlib.h>
#include "libfactorization.h"

#define INITIAL_CAPACITY 50

int factorize(unsigned short num, unsigned short **factors, int *factors_len)
{
    unsigned short *ret = NULL;
    int ret_num = 0;
    int capacity = INITIAL_CAPACITY;

    ret = calloc(capacity, sizeof(unsigned long long));
    if (ret == NULL)
    {
        fprintf(stderr, "cannot allocate\n");
        return 1;
    }

    unsigned short i = 2;
    while (i <= num)
    {
        if ((num % i) == 0)
        {
            ret[ret_num++] = i;
            if (ret_num >= capacity)
            {
                capacity += INITIAL_CAPACITY;
                ret = realloc(ret, capacity);
                if (ret == NULL)
                {
                    fprintf(stderr, "cannot reallocate\n");
                    return 1;
                }
            }
            num = num / i;
            continue;
        }
        i++;
    }

    *factors = ret;
    *factors_len = ret_num;

    return 0;
}