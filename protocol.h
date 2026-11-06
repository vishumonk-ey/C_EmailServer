
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

/* Protocol identification */
#define VMP_MAGIC_1 'V'
#define VMP_MAGIC_2 'M'

#define VMP_VERSION 1

/* Header size */
#define VMP_HEADER_SIZE 8

/* Maximum payload */
#define VMP_MAX_PAYLOAD 65536

/* Request types */
#define REQ_REGISTER 0x01
#define REQ_LOGIN    0x02
#define REQ_SEND     0x03
#define REQ_INBOX    0x04
#define REQ_READ     0x05
#define REQ_SENT     0x06
#define REQ_LOGOUT   0x07

/* Response types */
#define RESP_LOGIN_OK     0x81
#define RESP_LOGIN_FAILED 0x82
#define RESP_OK           0x83
#define RESP_ERROR        0xFF

/* Decoded message */
typedef struct {
    uint8_t version;
    uint8_t type;
    uint32_t payload_length;
    uint8_t *payload;
} VMPFrame;

/* Protocol functions */

/* Send and receive complete byte sequences */
int send_all(int sock, const void *buffer, size_t length);

int recv_all(int sock, void *buffer, size_t length);

/* Send and receive complete protocol frames */
int vmp_send_frame(
    int sock,
    uint8_t type,
    const uint8_t *payload,
    uint32_t payload_length
);

int vmp_recv_frame(int sock, VMPFrame *frame);

int vmp_free_frame(VMPFrame *frame);

/* Payload encoding */
int vmp_append_string(
    uint8_t *buffer,
    size_t capacity,
    size_t *offset,
    const char *value
);

/* Payload decoding */
int vmp_read_string(
    const uint8_t *payload,
    uint32_t payload_length,
    size_t *offset,
    char **value
);

#endif
