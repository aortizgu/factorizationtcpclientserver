
#ifndef _COMMON_
#define _COMMON_

#define DEFAULT_SERVER "127.0.0.1"
#define DEFAULT_PORT_STR "8080"
#define DEFAULT_PORT 8080
#define MAX_NUMBERS 10
#define MAX_FACTORIZABLE_NUMBER 50000

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

int isNum(char *string);
int parsePort(char *string);

#endif