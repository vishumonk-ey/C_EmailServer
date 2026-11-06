#include "protocol.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>  
#include <arpa/inet.h>   
#include <unistd.h>  

#define PORT 5555

int sendLoginReq(int clientfd,const char *username , const char *password){
    uint8_t payload[VMP_MAX_PAYLOAD];
    size_t offset = 0;

    // creating the payload : 
    if(vmp_append_string(
        payload,
        sizeof(payload),
        &offset,
        username
    ) < 0){
        printf("username appending error! \n");
    }

    if(vmp_append_string(
        payload,
        sizeof(payload),
        &offset,
        password
    ) < 0){
        printf("username appending error! \n");
    }


    if(vmp_send_frame(clientfd , REQ_LOGIN , payload , (uint32_t)offset) < 0){
        printf("Error while sending frame\n");
    }

    // recieve response - create a new function for recieving response !
    return 1;
}

int main(){
    int client_fd = socket(AF_INET , SOCK_STREAM , 0 );
    if ( client_fd < 0 ){
        printf("error while creating the client ! \n");
    }
    struct sockaddr_in serveraddr;
    serveraddr.sin_family  = AF_INET;
    serveraddr.sin_port = htons(PORT);
    inet_pton(AF_INET , "127.0.0.1" , &serveraddr.sin_addr);

    if(connect(client_fd , (struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0){
        printf("error while connecting\n");
    }
    const char *username = "Vishal";
    const char *password = "pwd123";
    sendLoginReq(client_fd , username , password);

}