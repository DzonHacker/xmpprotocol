#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
using namespace std;

#define SERVERPORT 1337
#define MAXLEN 100
#define INFOLEN 20
#define IDLEN 5
#define BUFFER_LEN 1024




// Packet Related 
enum STATUS_CODE {
    SUCCESS = 100,
    BAD_LEN = 50,
    BAD_COMMAND = 30,
    QUIT = 20,
    FAILURE = 80,
    PLAIN_LOGIN = 10,
    WRITE_SUCCESS = 120,
    READ_SUCCESS = 150,
    LOGIN_SUCCES = 200,
    WRITE_PERMIT = 40,
    SESSION_CLOSED = 1337,
    READ_OK = 60
};

// Messages 

struct EMPLOYEE_INFO {
    int id;
    char firstName[INFOLEN];
    char lastName[INFOLEN];
    char address[INFOLEN];
    char desc[INFOLEN];
};

struct SEND_DATA_STRUC {
    int status;
    char message[MAXLEN];
    struct EMPLOYEE_INFO eData;
};

struct MESSAGE_STRUC {
    int status;
    char message[MAXLEN];
};

int main(int argc, char **argv) {

    char buffer[BUFFER_LEN];
    char message[MAXLEN];
    int sock, conn;
    int bytesRead;
    struct sockaddr_in servAddr;
    struct MESSAGE_STRUC sendMessage;
    struct MESSAGE_STRUC *recvMessage;
    struct EMPLOYEE_INFO eInfo;
    struct SEND_DATA_STRUC *recvDataStruct;
    struct EMPLOYEE_INFO *recvEmpInfo;
    socklen_t len;

    // clearing servAddr
    memset(&servAddr,0, sizeof(servAddr));
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(SERVERPORT);
    servAddr.sin_family = AF_INET;

    // creating Stream socket
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("sock error\n");
        exit(0);
    }
    if (connect(sock, (struct sockaddr*)&servAddr, sizeof(servAddr))!=0) {
        perror("connect failed\n");
        exit(0);
    }

    int authed = 0;
    int status = 0;
    while(1) {
        // if not authenticated
        while(authed == 0) {
            memset(buffer, 0, BUFFER_LEN);
            printf("[*] command: ");
            fgets(message, MAXLEN, stdin);
            sendto(sock, message, strlen(message), 0, 
                 (struct sockaddr*)&servAddr, sizeof(servAddr));
            // memset(&recvMessage, 0, sizeof(struct MESSAGE_STRUC));
            recvfrom(sock, buffer, sizeof(struct MESSAGE_STRUC), 0, 
                (struct sockaddr*)&servAddr, &len);
            bytesRead = 1;
            if(bytesRead > 0) {
                recvMessage = (struct MESSAGE_STRUC*)&buffer;
                if(recvMessage->status == PLAIN_LOGIN) {
                    printf("[*] Server: status %d, %s\n",recvMessage->status, recvMessage->message);
                }else if(recvMessage->status == LOGIN_SUCCES) {
                    printf("[*] Server: status %d, %s\n",recvMessage->status, recvMessage->message);
                    authed = 1;
                }else {
                    printf("[*] Server: status %d, %s\n",recvMessage->status, recvMessage->message);
                }
            }
        }

        // if authenticated
        while(authed == 1) {
            memset(buffer, 0, BUFFER_LEN);
            printf("[*] Command: ");
            fgets(message, MAXLEN, stdin);
            sendto(sock, message, strlen(message), 0, 
                 (struct sockaddr*)&servAddr, sizeof(servAddr));
            // bytesRead = recvfrom(sock, &buffer, sizeof(struct MESSAGE_STRUC), 0, 
            //     (struct sockaddr*)&servAddr, &len);
            if(strncmp(message, "READ", 4) == 0) {
                bytesRead = recv(sock ,&buffer, sizeof(struct SEND_DATA_STRUC), 0);
            }else {
                bytesRead = recv(sock, &buffer, sizeof(struct MESSAGE_STRUC), 0);
            }

            // Write Process
            if(bytesRead > 0) {
                recvMessage = (struct MESSAGE_STRUC*)&buffer;
                // printf("[*] Recv Status %s\n",recvMessage->status);
                if(recvMessage->status == WRITE_PERMIT) {
                    printf("[*] ========================================= \n");
                    printf("[*] id: ");
                    scanf("%d", &eInfo.id);
                    getchar();
                    printf("[*] FirstName: ");
                    fgets(eInfo.firstName, INFOLEN, stdin);
                    printf("[*] LastName: ");
                    fgets(eInfo.lastName, INFOLEN, stdin);
                    printf("[*] Address: ");
                    fgets(eInfo.address, INFOLEN, stdin);
                    printf("[*] Desc: ");
                    fgets(eInfo.desc, INFOLEN, stdin);
                    printf("[*] ========================================= \n");
                    send(sock, &eInfo, sizeof(struct EMPLOYEE_INFO), 0);
                    recvfrom(sock, buffer, sizeof(struct MESSAGE_STRUC), 0, 
                        (struct sockaddr*)&servAddr, &len);
                    recvMessage = (struct MESSAGE_STRUC*)&buffer;
                    if(recvMessage->status == WRITE_SUCCESS) {
                        printf("[*] Server: status %d, %s\n",recvMessage->status, recvMessage->message);
                    }
                    bytesRead = 0;
                } else {
                    printf("[*] Server: status %d, %s\n",recvMessage->status, recvMessage->message);
                }

                // Read Process
                if(recvMessage->status == READ_OK) {
                    recvDataStruct = (struct SEND_DATA_STRUC*) &buffer;
                    recvEmpInfo = (struct EMPLOYEE_INFO*) &recvDataStruct->eData;
                    printf("[*] ========================================= \n");                    
                    printf("[*] Requesting Employee Details..\n");
                    printf("[*] Received Employee Details\n");   
                    printf("[*] Employee Id: %d\n", recvDataStruct->eData.id);
                    printf("[*] Employee Name: %s", recvDataStruct->eData.firstName);
                    printf("[*] Employee SurName: %s", recvEmpInfo->lastName);
                    printf("[*] Employee Address: %s", recvEmpInfo->address);
                    printf("[*] Employee Desc: %s", recvEmpInfo->desc);
                    printf("[*] ========================================= \n");
                    printf("[*] Server: status %d, %s\n",recvDataStruct->status, recvDataStruct->message);
                }
            }
        }
    }
}