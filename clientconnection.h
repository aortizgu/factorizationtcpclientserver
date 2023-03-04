#ifndef _CLIENTCONNECTION_
#define _CLIENTCONNECTION_

/**
 * @brief Request to a given server the factorization of a list of numbers and waits for the responses
 * with the factors of the numbers
 */
void clientconnection(const char *server, unsigned short port, unsigned short *numbers, int numbers_count);

#endif