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

typedef struct rspmsg_ *responsemessage;

typedef struct msg_ *message;
typedef struct msgprocessor_ *messageprocessor;

responsemessage rspmsg_init();
responsemessage rspmsg_init_from_msg(message);
void rspmsg_destroy(message);

requestmessage rqtmsg_init(unsigned short *numbers, int numbers_len);
int rqtmsg_serialize(requestmessage, char **buffer, int *buff_len);
requestmessage rqtmsg_init_from_msg(message);
void rqtmsg_destroy(requestmessage);

void msg_destroy(message);
int msg_get_msg_type(message);

messageprocessor msgprocessor_init();
void msgprocessor_destroy(messageprocessor);

void msgprocessor_add_raw_bytes(messageprocessor, char *buffer, const int buffer_len);
int msgprocessor_get_message(messageprocessor msgprocessor_ptr, message *new_message);
int msgprocessor_get_available_messages_count(messageprocessor);
message msgprocessor_get_next_message(messageprocessor);

#endif