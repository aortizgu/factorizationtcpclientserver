#ifndef _MESSAGEPROCESSOR_
#define _MESSAGEPROCESSOR_

#define REQUEST_MSG_ID 1
#define RESPONSE_MSG_ID 1

typedef struct rspmsg_ *responsemessage;
typedef struct rqtmsg_ *requestmessage;
typedef struct msg_ *message;
typedef struct msgprocessor_ *messageprocessor;

responsemessage rspmsg_init();
requestmessage rqtmsg_init();
responsemessage rspmsg_init(message);
requestmessage rqtmsg_init(message);

messageprocessor msgprocessor_init();
void msgprocessor_add_raw_bytes(messageprocessor, char *buffer, const int buffer_len);
int msgprocessor_get_available_messages_count(messageprocessor);
message msgprocessor_get_next_message(messageprocessor);
void msg_destroy(message);
void rspmsg_destroy(message);
void rqtmsg_destroy(message);
void msgprocessor_destroy(messageprocessor);

#endif