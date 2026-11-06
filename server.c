#include <iso646.h>
#define _POSIX_C_SOURCE 200809L

#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <limits.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>

#define PORT 5555

#define REQ_LOGIN       0x02
#define REQ_LOGOUT      0x07

#define RES_LOGIN_OK      0x81
#define RES_LOGIN_FAILED  0x82
#define RES_OK            0x83
#define RES_ERROR         0xFF

#define USERNAME_SIZE 64
#define PASSWORD_SIZE 128

#define DATABASE_PATH "Database/Users"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int send_response(
    int client_fd , 
    uint8_t response_type , 
    char *message 
){
    uint8_t *payload = NULL;
    uint32_t payload_length = 0;

    if (message != NULL) {
        payload_length = (uint32_t)strlen(message);
        payload = (uint8_t *)message;
    }

    return vmp_send_frame(client_fd, response_type, payload, payload_length);
}

//login handler
void handle_login(int client_fd , VMPFrame *frame ){
    size_t offset = 0;
    char *username = NULL;
    char *password = NULL;
    // int logged_in = 0;
    printf("Recieved req for login\n");
    // read username because it would be in byte format
    if(vmp_read_string(frame->payload , frame->payload_length , &offset , &username) < 0){
        printf("error occured while reading the username !\n");
        send_response(client_fd , RES_LOGIN_FAILED , "invalid request");
        // goto cleanup;
    }

    // read pwd as it will be present in byte format
    if(vmp_read_string(frame->payload , frame->payload_length , &offset , &password) < 0){
        printf("error occured while reading the password!\n");
        send_response(client_fd , RES_LOGIN_FAILED , "invalid request");
    }

    // build the user directory path: Database/users/<username>
    char user_dir[PATH_MAX];
    int n = snprintf(user_dir , sizeof(user_dir) , "%s/%s" , DATABASE_PATH , username);

    // check whether the user directory exists
    struct stat st;
    if (stat(user_dir , &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("user directory does not exist: %s\n", user_dir);
        send_response(client_fd , RES_LOGIN_FAILED , "user not found");
    }

    // build the credentials file path: Database/users/<username>/credentials
    char cred_path[PATH_MAX];
    n = snprintf(cred_path , sizeof(cred_path) , "%s/credentials" , user_dir);
    if (n < 0 || (size_t)n >= sizeof(cred_path)) {
        send_response(client_fd , RES_LOGIN_FAILED , "credentials dont exists !");
    }

    // open the credentials file and check the third line (the password)
    FILE *fp = fopen(cred_path , "r");
    if (fp == NULL) {
        printf("could not open credentials file: %s\n", cred_path);
        send_response(client_fd , RES_LOGIN_FAILED , "Error");
    }

    char line[256];
    int line_no = 0;
    char stored_pwd[256] = {0};
    pthread_mutex_lock(&mutex);
    while (fgets(line , sizeof(line) , fp) != NULL) {
        line_no++;
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (line_no == 3) {
            strncpy(stored_pwd , line , sizeof(stored_pwd) - 1);
            stored_pwd[sizeof(stored_pwd) - 1] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
    fclose(fp);

    if (line_no < 3) {
        printf("credentials file has no third line for user %s\n", username);
        send_response(client_fd , RES_LOGIN_FAILED , "invalid credentials");
        // goto cleanup;
    }

    // compare the supplied password with the third line of the file
    if (strcmp(stored_pwd , password) != 0) {
        printf("password mismatch for user %s\n", username);
        send_response(client_fd , RES_LOGIN_FAILED , "invalid password");
    }else{
        printf("Login request came from %s with pwd %s and was success !\n",username , password);
        send_response(client_fd , RES_LOGIN_OK , "login successful");
    }
    

}

// register handler
int handle_register(int client_fd , VMPFrame *frame){
    char *username = NULL;
    char *email = NULL;
    char *password = NULL;

    size_t offset = 0;

    if(vmp_read_string(frame->payload,frame->payload_length,&offset ,&username) < 0){
        printf("error occured while reading the username\n");
        send_response(client_fd, RES_LOGIN_FAILED, "request failed ");
        return -1;
    }

    if(vmp_read_string(frame->payload,frame->payload_length,&offset ,&email) < 0){
        printf("error occured while reading the email\n");
        send_response(client_fd, RES_LOGIN_FAILED, "request failed ");
        return -1;
    }

    if(vmp_read_string(frame->payload,frame->payload_length,&offset ,&password) < 0){
        printf("error occured while reading the password\n");
        send_response(client_fd, RES_LOGIN_FAILED, "request failed ");
        return -1;
    }

    // create a path Database/Users/username and check exists or not . If not then create a folder username in Database/Users

    char user_dir[PATH_MAX];
    int n = snprintf(user_dir , sizeof(user_dir) , "%s/%s" , DATABASE_PATH , username);

    // check whether the user directory exists
    struct stat st;
    if (stat(user_dir , &st) == 0 || S_ISDIR(st.st_mode)) {
        printf("user already exist: %s\n", user_dir);
        send_response(client_fd , RES_LOGIN_FAILED , "user already exists");
    }else{
        // create a folder in the path Database/Users named username . then in username folder add a credential file and write the credentials in it .

        int result = mkdir(user_dir,0777);
        if(result == 0){
            printf("User folder successfully created\n");
        }else{
            printf("User folder not created \n");
            return -1;
        }

        // creating a credentials file
        char cred_filepath[PATH_MAX];
        snprintf(cred_filepath , sizeof(cred_filepath) , "%s/%s" , user_dir , "credentials");
        FILE *file_ptr = fopen(cred_filepath,"w");
        if(file_ptr == NULL){
            printf("error while creating the file ptr\n");
            return -1;
        }
        pthread_mutex_lock(&mutex);
        fprintf(file_ptr ,"%s\n" , username);
        fflush(file_ptr);

        fprintf(file_ptr ,"%s\n", email);
        fflush(file_ptr);

        fprintf(file_ptr ,"%s\n", password);
        
        fclose(file_ptr);

        pthread_mutex_unlock(&mutex);

        send_response(client_fd, RESP_LOGIN_OK, "User Successfully Registered");

    }


}
void *client_handler(void *args){
    // printf("thread routine called!\n");
    int client_fd = *(int *)args;
    free(args);

    while(1){
        // printf("entered in while\n");
        VMPFrame frame;
        int result = vmp_recv_frame(client_fd , &frame);
        printf("result : %d",result);
        if ( result == -1){
            printf("Client recieve failed !\n");
        }else{
            printf("Client frame recieved successfully!\n");
        }
        
        switch(frame.type){
            case REQ_LOGIN:
                // printf("handle login matched in case\n");
                handle_login(client_fd , &frame);
                break;

            case REQ_REGISTER:
                handle_register(client_fd, &frame);
                break;
            default:
                send_response(client_fd , RES_ERROR , NULL);
                break;
        }
    }
}

int main(){
    printf("HEYYYY\n");
    int serverfd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(serverfd , (struct sockaddr*)&serveraddr , sizeof(serveraddr)) < 0){
        printf("error while binding\n");
        return -1 ;
    }
    
    if(listen(serverfd , 5 ) < 0){
        printf("error while listening!\n");
    }

    printf("Server successfully running on port %d\n" ,PORT);

    while(1){
        struct sockaddr_in clientaddr;
        socklen_t client_len = sizeof(clientaddr);

        int clientfd = accept(serverfd , (struct sockaddr *)&clientaddr , &client_len);
        if(clientfd>0){
            printf("client accepted successfully!\n");
        }
        int *thread_args = malloc(sizeof(clientfd));
        *thread_args = clientfd;

        pthread_t thread ;

        if (pthread_create(
            &thread ,
            NULL ,
            client_handler,
            thread_args 
        ) != 0 ){
            printf("error while creating thread!\n");
            free(thread_args);
            return -1;
        }
        // pthread_join(thread);  -- will basically wait until thread terminates and then reaps it and frees the resources !
        pthread_detach(thread);
    }
    close(serverfd);
    pthread_mutex_destroy(&mutex);
}
