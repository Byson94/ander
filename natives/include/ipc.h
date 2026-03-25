#pragma once
#include <stdint.h>

#define SOCKET_PATH  "/tmp/ander-launcher.sock"

#define MSG_ERROR              0x01
#define MSG_CALL               0x02
#define MSG_RETURN             0x03
#define MSG_JNIENV             0x04
#define MSG_JNIENV_RETURN      0x05
#define MSG_JNIENV_RETURN_DATA 0x06
#define MSG_LIST_SYMBOLS       0x07
#define MSG_ARRAY_DATA         0x08 
#define MSG_ARRAY_ACK          0x09 

#define BUF_SIZE 65536

typedef struct {
    uint8_t  type;
    uint32_t id;
    uint32_t data_len;
} __attribute__((packed)) MsgHeader;

extern int      g_sock;
extern uint32_t g_call_id;

int  write_all(int fd, const void *buf, size_t len);
int  read_all(int fd, void *buf, size_t len);
int  send_msg(uint8_t type, uint32_t id, const void *data, uint32_t len);
int  recv_msg_dyn(MsgHeader *hdr, uint8_t **buf_out);