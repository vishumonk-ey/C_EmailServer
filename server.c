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

#define DATABASE_PATH "Database/users"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int send_response(
    int client_fd , 
    uint8_t response_type , 
    char *message 
){

    
}


int handle_login(){

}

void *client_handler(void *args){
    int client_fd = *(int *)args;
    free(args);

    while(1){
        VMPFrame frame;
        int result = vmp_recv_frame(client_fd , &frame);
        if ( result == -1){
            printf("Client recieve failed !");
        }
        
        switch(frame.type){
            case REQ_LOGIN:
                handle_login(
                    
                )
                break;
            default:
                send_response(client_fd ,);
                break;
        }
    }
}

int main(){
    int serverfd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(serverfd , (struct sockaddr*)&server_addr , sizeof(server_addr)) < 0){
        printf("error while binding");
        return -1 ;
    }
    
    if(listen(serverfd , 5 ) < 0){
        printf("error while listening!");
    }

    while(1){
        struct sockaddr_in clientaddr;
        socklen_t client_len = sizeof(clientaddr);

        int clientfd = accept(serverfd , (struct sockaddr *)&clientaddr , &client_len);

        int *thread_args = malloc(sizeof(clientfd));
        *thread_args = client_fd;

        pthread_t thread ;

        if (pthread_create(
            &thread ,
            NULL ,
            client_handler,
            thread_args 
        ) != 0 ){
            printf("error while creating thread!");
            free(thread_args);
            return -1;
        }
        // pthread_join(thread);  -- will basically wait until thread terminates and then reaps it and frees the resources !
        pthread_detach(thread);
    }
    close(serverfd);
    pthread_mutex_destroy(&mutex);
}