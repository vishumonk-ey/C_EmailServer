#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>


/*
 * Sends exactly 'length' bytes.
 *
 * Returns:
 *  0  -> Success
 * -1  -> Error
 */


int send_all(int sock, const void *buffer, size_t length){
    const uint8_t *ptr = (const uint8_t *)buffer;
    size_t total_sent = 0 ;
    while(total_sent < length){
        size_t sent = send(
            sock ,
            ptr + total_sent ,
            length - total_sent , 
            0
        )  
        
        if ( sent < 0 ){
            if (errno == EINTR){
                continue;
            }
            return -1;
        }
        total_sent += sent
    }
    return -1;
}

/*
 * Receives exactly 'length' bytes.
 *
 * Returns:
 *  0  -> Success
 * -1  -> Error or incomplete data
 */

 int recv_all(int sock, void *buffer, size_t length){
    uint8_t *ptr = (uint8_t *)buffer;
    size_t total_recieved = 0;
    while( total_recieved < length){
        size_t recieved = recv(
            sock , 
            ptr + total_recieved ,
            length - total_recieved ,
            0
        )

        if (recieved < 0){
            if (errno == EINTR){
                continue;
            }
            return -1;
        }
    }
    return 0;
 }


 /*
 * Sends one complete VMP message.
 */

int vmp_send_frame(
    int sock,
    uint8_t type,
    const uint8_t *payload,
    uint32_t payload_length
){
    if ( payload_length > VMP_MAX_PAYLOAD ){
        errno = EMSGSIZE;
        return -1
    }

    if ( payload_length > 0 && payload == NULL){
        errno = EINVAL;
        return -1 ;
    }

    uint8_t header[VMP_HEADER_SIZE];
    header[0] = VMP_MAGIC_1;
    header[1] = VMP_MAGIC_2;

    header[2] = VMP_VERSION;

    header[3] = type;

    uint32_t network_length = htonl(payload_length);

    memcpy(
        header + 4 , 
        &network_length ,
        sizeof(network_length)
    );

    // sending header first
    if ( send_all(sock , header , VMP_HEADER_SIZE) < 0){
        return -1;
    }
    // send the payload
    if ( send_all(sock,payload,payload_length) < 0){
        return -1;
    }


    return 0;
}

int vmp_recv_frame(int sock, VMPFrame *frame) {
    if ( frame == NULL ){
        return -1;
    }
    // initializing the frame
    memset(frame,0,sizeof(*frame)) ;
    uint8_t header[VMP_HEADER_SIZE];

    if( recv_all(sock , header , VMP_HEADER_SIZE) < 0){
        return -1;
    }

    // validate whether it is VMP message or not

    if( header[0] != VMP_MAGIC_1 || header[1] != VMP_MAGIC_2){
        errno = EPROTO;
        return -1;
    }
    // check protocol version
    if( header[2] != VMP_VERSION){
        errno = EPROTONOSUPPORT;
        return -1;
    }
    uint32_t payload_length ;
    memcpy(
        &payload_length ,
        header+4,
        sizeof(payload_length)
    );

    payload_length = ntohl(payload_length);

    uint8_t *payload = NULL;
    
    if( payload_length > VMP_MAX_PAYLOAD){
        errno = EMSGSIZE;
        return -1;
    }
    // recieve payload only when the payload is existing !

    if(payload_length > 0 ){
        payload = malloc(payload_length);

        if(payload == NULL){
            return -1;
        }
        
        if( recv_all(sock , payload , payload_length) < 0){
            return -1;
        }
    }

    // populate the frame :

    frame->version = header[2];
    frame->type = header[3];
    frame->payload_length = payload_length;
    frame->payload = payload;

    return 1;

}

int vmp_free_frame(VMPFrame *frame){
    if( frame == NULL){
        return -1;
    }
    free(frame->payload)
    return 1;
}



/*
 * Adds one string to the payload buffer.
 *
 * Encoding:
 * [2-byte length][string bytes]
 */

 int vmp_append_string(
    uint8_t *buffer,
    size_t capacity,
    size_t *offset,
    const char *value
) {

    if (buffer == NULL || offset == NULL || value == NULL) {
        errno = EINVAL;
        return -1;
    }

    size_t length = strlen(value);

    /* Field length is represented using 2 bytes */
    if (length > UINT16_MAX) {
        errno = EMSGSIZE;
        return -1;
    }

    /* Ensure enough buffer space */
    if (*offset > capacity ||
        capacity - *offset < 2 ||
        capacity - *offset - 2 < length) {

        errno = ENOBUFS;
        return -1;
    }

    /* Convert field length to network byte order */
    uint16_t network_length = htons((uint16_t)length);

    memcpy(buffer + *offset, &network_length, 2);

    *offset += 2;

    /* Copy actual string bytes */
    memcpy(buffer + *offset, value, length);

    *offset += length;

    return 0;
}


/*
 * Reads one string from the payload.
 *
 * Allocates memory for the decoded string.
 * Caller must free the returned string.
 */

int vmp_read_string(
    const uint8_t *payload,
    uint32_t payload_length,
    size_t *offset,
    char **value
) {

    if (payload == NULL || offset == NULL || value == NULL) {
        errno = EINVAL;
        return -1;
    }

    *value = NULL;

    /* Ensure 2 bytes are available for field length */
    if (*offset > payload_length ||
        payload_length - *offset < 2) {

        errno = EPROTO;
        return -1;
    }

    uint16_t network_length;

    memcpy(
        &network_length,
        payload + *offset,
        2
    );

    uint16_t length = ntohs(network_length);

    *offset += 2;

    /* Check whether field data fits inside payload */
    if (length > payload_length - *offset) {

        errno = EPROTO;
        return -1;
    }

    /* We are decoding C strings, so reject embedded NUL bytes */
    if (memchr(payload + *offset, '\0', length) != NULL) {

        errno = EPROTO;
        return -1;
    }

    /* Allocate space for string + terminating null character */
    char *result = malloc((size_t)length + 1);

    if (result == NULL)
        return -1;

    memcpy(result, payload + *offset, length);

    result[length] = '\0';

    *offset += length;

    *value = result;

    return 0;
}
