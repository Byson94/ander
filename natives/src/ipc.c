#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ipc.h"

int      g_sock    = -1;
uint32_t g_call_id = 0;

int write_all(int fd, const void *buf, size_t len) {
    const uint8_t *p = buf;
    while (len > 0) {
        ssize_t n = write(fd, p, len);
        if (n <= 0) return -1;
        p += n; len -= n;
    }
    return 0;
}

int read_all(int fd, void *buf, size_t len) {
    uint8_t *p = buf;
    while (len > 0) {
        ssize_t n = read(fd, p, len);
        if (n <= 0) return -1;
        p += n; len -= n;
    }
    return 0;
}

int send_msg(uint8_t type, uint32_t id, const void *data, uint32_t len) {
    MsgHeader hdr = { type, id, len };
    if (write_all(g_sock, &hdr, sizeof(hdr)) < 0) return -1;
    if (len > 0 && write_all(g_sock, data, len) < 0) return -1;
    return 0;
}

int recv_msg_dyn(MsgHeader *hdr, uint8_t **buf_out) {
    if (read_all(g_sock, hdr, sizeof(MsgHeader)) < 0) {
        fprintf(stderr, "[AnderBridge] recv_msg: header read failed\n");
        fflush(stderr);
        return -1;
    }
    fprintf(stderr, "[AnderBridge] recv_msg: type=0x%02x id=%u dlen=%u\n",
            hdr->type, hdr->id, hdr->data_len);
    fflush(stderr);
    if (hdr->data_len == 0) {
        *buf_out = NULL;
        return 0;
    }
    *buf_out = malloc(hdr->data_len);
    if (!*buf_out) {
        fprintf(stderr, "[AnderBridge] recv_msg: OOM for dlen=%u\n", hdr->data_len);
        fflush(stderr);
        return -1;
    }
    if (read_all(g_sock, *buf_out, hdr->data_len) < 0) {
        fprintf(stderr, "[AnderBridge] recv_msg: body read failed\n");
        fflush(stderr);
        free(*buf_out);
        *buf_out = NULL;
        return -1;
    }
    return 0;
}