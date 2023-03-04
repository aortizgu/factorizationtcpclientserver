#ifndef _MESSAGEPROCESSOR_
#define _MESSAGEPROCESSOR_

enum MSG_TYPE
{
    REQUEST_MSG_ID,
    RESPONSE_MSG_ID
};

typedef struct rqtmsg_
{
    unsigned short *numbers;
    unsigned char numbers_len;
} rqtmsg_;
typedef struct rqtmsg_ *requestmessage;

typedef struct rspmsg_
{
    unsigned short *factors;
    unsigned char factors_len;
    unsigned short number;
} rspmsg_;
typedef struct rspmsg_ *responsemessage;

typedef struct msg_ *message;
typedef struct msgprocessor_ *messageprocessor;

/**
 * @brief Initialize requestmessage from input parameters
 */
requestmessage rqtmsg_init(unsigned short *numbers, int numbers_len);
/**
 * @brief Initialize requestmessage from base message
 */
requestmessage rqtmsg_init_from_msg(message);
/**
 * @brief Serialize requestmessage into a buffer
 */
int rqtmsg_serialize(requestmessage, char **buffer, int *buff_len);
/**
 * @brief Release all requestmessage resources
 */
void rqtmsg_destroy(requestmessage);

/**
 * @brief Initialize responsemessage from input parameters
 */
responsemessage rspmsg_init(unsigned short number, unsigned short *factors, int factors_len);
/**
 * @brief Initialize responsemessage from base message
 */
responsemessage rspmsg_init_from_msg(message);
/**
 * @brief Serialize responsemessage into a buffer
 */
int rspmsg_serialize(responsemessage, char **buffer, int *buff_len);
/**
 * @brief Release all responsemessage resources
 */
void rspmsg_destroy(responsemessage);

/**
 * @brief Release all message resources
 */
void msg_destroy(message);
/**
 * @brief Get message type from base message
 */
unsigned char msg_get_msg_type(message);

/**
 * @brief Initialize internal messageprocessor resources
 */
messageprocessor msgprocessor_init();
/**
 * @brief Release all messageprocessor resources
 */
void msgprocessor_destroy(messageprocessor);
/**
 * @brief Adds raw bytes to the messageprocessor for internal management
 */
void msgprocessor_add_raw_bytes(messageprocessor, char *buffer, const int buffer_len);
/**
 * @brief Gets an available message from the message processor
 */
int msgprocessor_get_message(messageprocessor msgprocessor_ptr, message *new_message);
/**
 * @brief Get the num of available messages
 */
int msgprocessor_get_available_messages_count(messageprocessor);

#endif