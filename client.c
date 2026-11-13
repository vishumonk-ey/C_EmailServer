#include "protocol.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

#define PORT 5555
#define BUFFER_SIZE 256

/* Read a line from stdin, strip trailing newline/CR, return a malloc'd string.
 * Returns NULL on EOF or error. Caller must free(). */
static char *read_line(const char *prompt) {
    static char buf[BUFFER_SIZE];
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, sizeof(buf), stdin) == NULL) {
        return NULL;
    }
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
    char *result = malloc(len + 1);
    if (result != NULL) {
        strcpy(result, buf);
    }
    return result;
}

/* Receive one frame and verify its type matches expected_type.
 * If out_msg is non-null, copy the payload (null-terminated) into it.
 * Returns 0 on success, -1 on error. Frame is always freed. */
static int recv_frame_expect(int client_fd, uint8_t expected_type,
                             char *out_msg, size_t out_size) {
    VMPFrame frame;
    memset(&frame, 0, sizeof(frame));
    if (vmp_recv_frame(client_fd, &frame) < 0) {
        printf("Error receiving response from server\n");
        return -1;
    }
    if (frame.type != expected_type) {
        printf("Unexpected response type: 0x%02X (expected 0x%02X)\n",
               frame.type, expected_type);
        vmp_free_frame(&frame);
        return -1;
    }
    if (out_msg != NULL && out_size > 0) {
        if (frame.payload_length > 0 && frame.payload_length < out_size) {
            memcpy(out_msg, frame.payload, frame.payload_length);
            out_msg[frame.payload_length] = '\0';
        } else {
            out_msg[0] = '\0';
        }
    }
    vmp_free_frame(&frame);
    return 0;
}

/* ---- Request builders ---- */

int sendLoginReq(int clientfd, const char *username, const char *password) {
    uint8_t payload[VMP_MAX_PAYLOAD];
    size_t offset = 0;

    if (vmp_append_string(payload, sizeof(payload), &offset, username) < 0) {
        printf("username appending error! \n");
        return -1;
    }
    if (vmp_append_string(payload, sizeof(payload), &offset, password) < 0) {
        printf("password appending error! \n");
        return -1;
    }
    if (vmp_send_frame(clientfd, REQ_LOGIN, payload, (uint32_t)offset) < 0) {
        printf("Error while sending frame\n");
        return -1;
    }
    return 0;
}

int sendRegisterReq(int clientfd, const char *username,
                    const char *email, const char *password) {
    uint8_t payload[VMP_MAX_PAYLOAD];
    size_t offset = 0;

    if (vmp_append_string(payload, sizeof(payload), &offset, username) < 0) {
        printf("username appending error! \n");
        return -1;
    }
    if (vmp_append_string(payload, sizeof(payload), &offset, email) < 0) {
        printf("email appending error! \n");
        return -1;
    }
    if (vmp_append_string(payload, sizeof(payload), &offset, password) < 0) {
        printf("password appending error! \n");
        return -1;
    }
    if (vmp_send_frame(clientfd, REQ_REGISTER, payload, (uint32_t)offset) < 0) {
        printf("Error while sending frame\n");
        return -1;
    }
    return 0;
}

int sendEmailReq(int client_fd, char **recieverArr,
                 const char *subject, const char *body,
                 const char *senderUsername, int numRecievers) {
    char payload[VMP_MAX_PAYLOAD];
    size_t offset = 0;

    if (vmp_append_string(payload, VMP_MAX_PAYLOAD, &offset, senderUsername) < 0) {
        printf("senderUsername appending error! \n");
        return -1;
    }
    char numRecieversStr[16];
    snprintf(numRecieversStr, sizeof(numRecieversStr), "%d", numRecievers);
    if (vmp_append_string(payload, VMP_MAX_PAYLOAD, &offset, numRecieversStr) < 0) {
        printf("numRecievers appending error! \n");
        return -1;
    }
    for (int i = 0; i < numRecievers; i++) {
        if (vmp_append_string(payload, VMP_MAX_PAYLOAD, &offset, recieverArr[i]) < 0) {
            printf("reciever appending error! \n");
            return -1;
        }
    }
    if (vmp_append_string(payload, VMP_MAX_PAYLOAD, &offset, subject) < 0) {
        printf("subject appending error! \n");
        return -1;
    }
    if (vmp_append_string(payload, VMP_MAX_PAYLOAD, &offset, body) < 0) {
        printf("body appending error! \n");
        return -1;
    }
    if (vmp_send_frame(client_fd, REQ_SEND, payload, (uint32_t)offset) < 0) {
        printf("send frame error! \n");
        return -1;
    }
    return 0;
}

int sendInboxReq(int client_fd, const char *username) {
    uint8_t payload[VMP_MAX_PAYLOAD];
    size_t offset = 0;
    if (vmp_append_string(payload, VMP_MAX_PAYLOAD, &offset, username) < 0) {
        printf("username appending error! \n");
        return -1;
    }
    if (vmp_send_frame(client_fd, REQ_INBOX, payload, (uint32_t)offset) < 0) {
        printf("send frame error! \n");
        return -1;
    }
    return 0;
}

int sendSentReq(int client_fd, const char *username) {
    uint8_t payload[VMP_MAX_PAYLOAD];
    size_t offset = 0;
    if (vmp_append_string(payload, VMP_MAX_PAYLOAD, &offset, username) < 0) {
        printf("username appending error! \n");
        return -1;
    }
    if (vmp_send_frame(client_fd, REQ_SENT, payload, (uint32_t)offset) < 0) {
        printf("send frame error! \n");
        return -1;
    }
    return 0;
}

int sendReadReq(int client_fd, const char *mailId) {
    uint8_t payload[VMP_MAX_PAYLOAD];
    size_t offset = 0;
    if (vmp_append_string(payload, VMP_MAX_PAYLOAD, &offset, mailId) < 0) {
        printf("mail id appending error! \n");
        return -1;
    }
    if (vmp_send_frame(client_fd, REQ_READ, payload, (uint32_t)offset) < 0) {
        printf("send frame error! \n");
        return -1;
    }
    return 0;
}

/* ---- Auth flows ---- */

int do_register(int client_fd) {
    char *username = read_line("Enter username: ");
    char *email = read_line("Enter email: ");
    char *password = read_line("Enter password: ");
    if (username == NULL || email == NULL || password == NULL) {
        free(username); free(email); free(password);
        printf("Input error\n");
        return -1;
    }

    if (sendRegisterReq(client_fd, username, email, password) < 0) {
        free(username); free(email); free(password);
        return -1;
    }

    VMPFrame frame;
    memset(&frame, 0, sizeof(frame));
    if (vmp_recv_frame(client_fd, &frame) < 0) {
        printf("Error receiving response from server\n");
        free(username); free(email); free(password);
        return -1;
    }

    if (frame.type == RESP_LOGIN_OK) {
        char msg[256] = {0};
        if (frame.payload_length > 0 && frame.payload_length < sizeof(msg)) {
            memcpy(msg, frame.payload, frame.payload_length);
            msg[frame.payload_length] = '\0';
            printf("Registration successful: %s\n", msg);
        } else {
            printf("Registration successful\n");
        }
    } else if (frame.type == RESP_LOGIN_FAILED || frame.type == RESP_ERROR) {
        char msg[256] = {0};
        if (frame.payload_length > 0 && frame.payload_length < sizeof(msg)) {
            memcpy(msg, frame.payload, frame.payload_length);
            msg[frame.payload_length] = '\0';
            printf("Registration failed: %s\n", msg);
        } else {
            printf("Registration failed\n");
        }
    } else {
        printf("Unexpected response from server\n");
    }

    vmp_free_frame(&frame);
    free(username); free(email); free(password);
    return (frame.type == RESP_LOGIN_OK) ? 0 : -1;
}

int do_login(int client_fd, char **out_username) {
    char *username = read_line("Enter username: ");
    char *password = read_line("Enter password: ");
    if (username == NULL || password == NULL) {
        free(username); free(password);
        printf("Input error\n");
        return -1;
    }

    if (sendLoginReq(client_fd, username, password) < 0) {
        free(username); free(password);
        return -1;
    }

    char msg[256] = {0};
    if (recv_frame_expect(client_fd, RESP_LOGIN_OK, msg, sizeof(msg)) < 0) {
        printf("Login failed: %s\n", msg);
        free(username); free(password);
        return -1;
    }

    printf("You are successfully logged in!\n");
    *out_username = username; /* hand ownership to caller */
    free(password);
    return 0;
}

/* ---- Menu actions ---- */

int do_inbox(int client_fd, const char *username) {
    if (sendInboxReq(client_fd, username) < 0) {
        return -1;
    }

    VMPFrame frame;
    memset(&frame, 0, sizeof(frame));
    if (vmp_recv_frame(client_fd, &frame) < 0) {
        printf("Error receiving inbox\n");
        return -1;
    }
    if (frame.type != RESP_INBOX) {
        printf("Unexpected response: 0x%02X\n", frame.type);
        vmp_free_frame(&frame);
        return -1;
    }

    printf("\n----- Inbox -----\n");
    if (frame.payload_length > 0) {
        fwrite(frame.payload, 1, frame.payload_length, stdout);
        printf("\n");
    } else {
        printf("Inbox is empty.\n");
    }
    printf("-----------------\n");

    char *choice = read_line("Enter mail ID to read, or press Enter to go back: ");
    if (choice != NULL && choice[0] != '\0') {
        if (sendReadReq(client_fd, choice) < 0) {
            printf("Failed to send read request\n");
        } else {
            VMPFrame rframe;
            memset(&rframe, 0, sizeof(rframe));
            if (vmp_recv_frame(client_fd, &rframe) < 0) {
                printf("Error receiving mail content\n");
            } else if (rframe.type == RESP_OK) {
                printf("\n----- Mail Content -----\n");
                fwrite(rframe.payload, 1, rframe.payload_length, stdout);
                printf("\n------------------------\n");
            } else {
                printf("Failed to read mail (type 0x%02X)\n", rframe.type);
            }
            vmp_free_frame(&rframe);
        }
    }
    free(choice);
    vmp_free_frame(&frame);
    return 0;
}

int do_sent(int client_fd, const char *username) {
    if (sendSentReq(client_fd, username) < 0) {
        return -1;
    }

    VMPFrame frame;
    memset(&frame, 0, sizeof(frame));
    if (vmp_recv_frame(client_fd, &frame) < 0) {
        printf("Error receiving sent mails\n");
        return -1;
    }
    if (frame.type != RESP_OK) {
        printf("Unexpected response: 0x%02X\n", frame.type);
        vmp_free_frame(&frame);
        return -1;
    }

    printf("\n----- Sent Mails -----\n");
    if (frame.payload_length > 0) {
        fwrite(frame.payload, 1, frame.payload_length, stdout);
        printf("\n");
    } else {
        printf("No sent mails.\n");
    }
    printf("----------------------\n");

    vmp_free_frame(&frame);
    return 0;
}

int do_send_mail(int client_fd, const char *senderUsername) {
    char *numStr = read_line("Enter number of receivers: ");
    if (numStr == NULL) {
        printf("Input error\n");
        return -1;
    }
    int numRecv = atoi(numStr);
    free(numStr);
    if (numRecv <= 0) {
        printf("Invalid number of receivers\n");
        return -1;
    }

    char **recvArr = malloc((size_t)numRecv * sizeof(char *));
    if (recvArr == NULL) {
        printf("Memory error\n");
        return -1;
    }
    for (int i = 0; i < numRecv; i++) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "Enter receiver %d: ", i + 1);
        recvArr[i] = read_line(prompt);
        if (recvArr[i] == NULL) {
            for (int j = 0; j < i; j++) free(recvArr[j]);
            free(recvArr);
            printf("Input error\n");
            return -1;
        }
    }

    char *subject = read_line("Enter subject: ");
    char *body = read_line("Enter body: ");
    if (subject == NULL || body == NULL) {
        for (int i = 0; i < numRecv; i++) free(recvArr[i]);
        free(recvArr);
        free(subject); free(body);
        printf("Input error\n");
        return -1;
    }

    if (sendEmailReq(client_fd, recvArr, subject, body, senderUsername, numRecv) < 0) {
        printf("Error sending mail\n");
    } else {
        printf("Mail sent successfully!\n");
    }

    for (int i = 0; i < numRecv; i++) free(recvArr[i]);
    free(recvArr);
    free(subject);
    free(body);
    return 0;
}

int main(void) {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serveraddr.sin_addr) <= 0) {
        printf("Invalid address\n");
        close(client_fd);
        return -1;
    }

    if (connect(client_fd, (struct sockaddr *)&serveraddr,
                sizeof(serveraddr)) < 0) {
        perror("connect");
        close(client_fd);
        return -1;
    }

    /* ---- Authentication phase ---- */
    int authenticated = 0;
    char *username = NULL;

    while (!authenticated) {
        printf("\n=== Mail Client ===\n");
        printf("1. Register\n");
        printf("2. Login\n");
        printf("3. Exit\n");
        char *choice = read_line("Select option: ");
        if (choice == NULL) {
            break;
        }
        int opt = atoi(choice);
        free(choice);

        if (opt == 1) {
            do_register(client_fd);
        } else if (opt == 2) {
            if (do_login(client_fd, &username) == 0) {
                authenticated = 1;
            }
        } else if (opt == 3) {
            break;
        } else {
            printf("Invalid option\n");
        }
    }

    if (!authenticated || username == NULL) {
        free(username);
        close(client_fd);
        return 0;
    }

    /* ---- Main menu phase ---- */
    while (1) {
        printf("\n=== Main Menu (Logged in as %s) ===\n", username);
        printf("1. See Inbox\n");
        printf("2. See Sent\n");
        printf("3. Send Mail\n");
        printf("4. Logout\n");
        char *choice = read_line("Select option: ");
        if (choice == NULL) {
            break;
        }
        int opt = atoi(choice);
        free(choice);

        if (opt == 1) {
            do_inbox(client_fd, username);
        } else if (opt == 2) {
            do_sent(client_fd, username);
        } else if (opt == 3) {
            do_send_mail(client_fd, username);
        } else if (opt == 4) {
            printf("Goodbye!\n");
            break;
        } else {
            printf("Invalid option\n");
        }
    }

    free(username);
    close(client_fd);
    return 0;
}
