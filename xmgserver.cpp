#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;

#define MAXLEN 100
#define PORT 1337

#define USERNAME "admin"
#define PASSWORD "admin"
#define INFOLEN 20
#define IDLEN 5


// Status Code
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

// Payload / Data design
// Data to send and receive over wire
struct EMPLOYEE_INFO { // Payload Design
    int id;
    char firstName[INFOLEN];
    char lastName[INFOLEN];
    char address[INFOLEN];
    char desc[INFOLEN];
};

//Packet Structure
// Normal Packet Structure
struct MESSAGE_STRUC {
    int status;
    char message[MAXLEN];
};

// Packet Structure With Data
struct SEND_DATA_STRUC {
    int status;
    char message[MAXLEN];
    struct EMPLOYEE_INFO eData; //Payload
};

// Retrieve Info
void ParseData(char* src, char* dest, int index) {
    int len = strlen(src);
    for(int i = 0; i < len-index; i++) {
        dest[i] = src[index+i+1];
    }
}

// Write Function
int WriteToDisk(struct EMPLOYEE_INFO *employee) {
    fstream fp;
    size_t res;

    struct EMPLOYEE_INFO enfo;
    enfo.id = employee->id;
    strcpy(enfo.firstName, employee->firstName);
    strcpy(enfo.lastName, employee->lastName);
    strcpy(enfo.address, employee->address);
    strcpy(enfo.desc, employee->desc);

    fp.open("employee.dat", ios::out | ios::binary);
    if(fp.is_open()) {
        fp.write(reinterpret_cast<char*>(&enfo), sizeof(EMPLOYEE_INFO));
        fp.close();
        return WRITE_SUCCESS;
    }
    return 1;

}

// Read Function
struct EMPLOYEE_INFO ReadFromDisk() {
    // FILE *infp;
    fstream fp;
    size_t res;
    struct EMPLOYEE_INFO enfo; 
    char buffer[sizeof(struct EMPLOYEE_INFO)];
    // infp = fopen("abc.txt","rb");
    fp.open("employee.dat", ios::in | ios::binary);
    if(fp.is_open()) {
        fp.read(reinterpret_cast<char*> (&enfo), sizeof(struct EMPLOYEE_INFO));
        fp.close();
    }
    return enfo;
}

int main(int argc, char **argv) {

    int sockfd, newSockfd;
    struct sockaddr_in servAddr, clientAddr;
    struct EMPLOYEE_INFO employee_info;
    struct EMPLOYEE_INFO *recvedInfo;
    struct EMPLOYEE_INFO *sendInfo;
    struct MESSAGE_STRUC sendMesage;
    struct MESSAGE_STRUC *recvMesage;
    struct SEND_DATA_STRUC sendData;
    char message[MAXLEN];
    char myMessage[MAXLEN];
    char tmpBuf[MAXLEN];
    int yes; 
    
    //socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[*] Socket Error\n");
        exit(0);
    }
    // zero out the struct
    memset(&servAddr, 0, sizeof(servAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));
     if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    // server configruartion
    servAddr.sin_family = AF_INET;  // family IPv4
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(PORT);

       // binding socket with server address
    if(bind(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0) {
        perror("[*] failed to bind");
        exit(EXIT_FAILURE);
    }
    if((listen(sockfd, 4)) != 0) { // listen to port 1337
        perror("listen failed\n");
        exit(0);
    } 
    socklen_t len = sizeof(clientAddr);
    // accept data packet from client
    if((newSockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &len)) < 0) {
        perror("accept failed\n");
        exit(0);
    }

    int bytesReceived, bytesSend;
    socklen_t rLen;
    char *tmpUser = (char*)malloc(20);
    char *tmpPass = (char*)malloc(50);
    char employName[20];
    int authed = 0; // checking if authenticated or not
    int user = 0; // checking if USER command has been executed or not
    int auth = 0; // checking if AUTH command has been executed or not
    
    // program logic
    while(1) {
        memset(&sendMesage, 0, sizeof(struct MESSAGE_STRUC));
        memset(&recvMesage, 0, sizeof(struct MESSAGE_STRUC));
        memset(&sendData, 0, sizeof(struct SEND_DATA_STRUC));
        memset(&message, 0, strlen(message));
        memset(myMessage, 0,MAXLEN);
        bytesReceived = recvfrom(newSockfd, &message, MAXLEN, 0, (struct sockaddr*)&clientAddr, &rLen);
        // checking if the client is authenticated or not
        // Authentication Logic
        if(bytesReceived > 0 && authed == 0) { // if authed is 0, not loggedin
            // Checking if the Auth has started
            if(strncmp(message, "AUTH", 4) == 0 && user == 0) {
                sendMesage.status = PLAIN_LOGIN;
                printf("[*] Status: Client is ready to Login\n");
                strcpy(myMessage, "Ready to Login. Please enter your credentials");
                myMessage[strlen(myMessage)] = '\0';
                strncpy(sendMesage.message, myMessage, strlen(myMessage));
                bytesSend = sendto(newSockfd, &sendMesage, sizeof(struct MESSAGE_STRUC), 0, 
                    (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                if(bytesSend > 0) {
                    auth = 1;
                }
            }
            // Checking if the client has sent USER command
            if(strncmp(message, "USER", 4) == 0 && user==0 && auth == 1) {
                printf("[*] Status: Client Entered Username\n");
                ParseData(message, tmpUser, 4);
                memset(myMessage, 0, strlen(myMessage));
                memset(&sendMesage, 0, sizeof(struct MESSAGE_STRUC));
                sendMesage.status = SUCCESS;
                strcpy(myMessage,"ENTER PASSWORD");
                myMessage[strlen(myMessage)] = '\0';
                strncpy(sendMesage.message, myMessage, strlen(myMessage));
                sendto(newSockfd, &sendMesage, sizeof(struct MESSAGE_STRUC), 0, 
                    (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                user = 1;           
            }
            // Checking if the client has sent PASS command
            // And checking if USER / PASS is correct or not
            // if correct auth success else re-enter USER / PASS
            if(strncmp(message, "PASS", 4) == 0 && auth == 1) {
                // Checking if client has Entered PASS command before USER command
                if(user == 0) {
                    sendMesage.status = FAILURE;
                    strcpy(myMessage,"Enter USER First");
                    myMessage[strlen(myMessage)] = '\0';
                    strncpy(sendMesage.message, myMessage, strlen(myMessage));
                    // This line will cause error
                    sendto(newSockfd, &sendMesage, sizeof(struct MESSAGE_STRUC), 0, 
                        (struct sockaddr*)&clientAddr, sizeof(clientAddr));  
                }else {
                    ParseData(message, tmpPass, 4);
                    printf("[*] Status: Client Entered Password\n");
                    printf("[*] Status: Authenticating...\n");
                    // Checking if username and password is correct
                    if(strncmp(tmpUser, USERNAME,strlen(USERNAME)) == 0 &&
                        strncmp(tmpPass, PASSWORD,strlen(PASSWORD))==0) {
                        // if USER/PASS is correct
                        printf("[*] Status: Login Successful...\n");
                        sendMesage.status = LOGIN_SUCCES;
                        strcpy(myMessage,"Login Successful");
                        myMessage[strlen(myMessage)] = '\0';
                        strncpy(sendMesage.message, myMessage, strlen(myMessage));
                        sendto(newSockfd, &sendMesage, sizeof(struct MESSAGE_STRUC), 0, 
                            (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                        authed = 1;
                    }else {
                        // if incorrect USER/PASS
                        sendMesage.status = FAILURE;
                        printf("[*] Status: Login Failed, invalid USER/PASS...\n");
                        strcpy(myMessage,"Incorrect USER/PASS.");
                        myMessage[strlen(myMessage)] = '\0';
                        strncpy(sendMesage.message, myMessage, strlen(myMessage));
                        sendto(newSockfd, &sendMesage, sizeof(struct MESSAGE_STRUC), 0, 
                            (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                        user = 0;
                    }
                }
            }
        }
                // sendMesage.status = WRITE_PERMIT;

        if(authed == 1) {
            if(strncmp(message, "WRITE", 4) == 0) {
                // Write Process
                sendMesage.status = WRITE_PERMIT;
                strcpy(myMessage,"Fill Employee Details");
                strncpy(sendMesage.message, myMessage, strlen(myMessage));
                sendto(newSockfd, &sendMesage, sizeof(struct MESSAGE_STRUC), 0, 
                    (struct sockaddr*)&clientAddr, sizeof(clientAddr));  
                // memset(&message, 0, strlen(message));
                printf("sent message %s\n",sendMesage.message);
                bytesReceived = 0;
                while (bytesReceived == 0) {
                    bytesReceived = recvfrom(newSockfd, &message, sizeof(struct EMPLOYEE_INFO), 0, 
                        (struct sockaddr*)&clientAddr, &rLen);
                    recvedInfo = (struct EMPLOYEE_INFO*)&message;
                }
                printf("[*] Reading From disk...\n");
                int written = WriteToDisk(recvedInfo);
                if(written == WRITE_SUCCESS) {
                    printf("[*] Write Success...\n");
                    sendMesage.status = WRITE_SUCCESS;
                    strcpy(myMessage,"Record written to file.");
                    myMessage[strlen(myMessage)] = '\0';
                    strncpy(sendMesage.message, myMessage, strlen(myMessage));
                    sendto(newSockfd, &sendMesage, sizeof(struct MESSAGE_STRUC), 0, 
                        (struct sockaddr*)&clientAddr, sizeof(clientAddr));  
                }
            }

            if(strncmp(message, "READ", 4) == 0) {
                // Read Process
                printf("[*] Reading From disk...\n");
                employee_info = ReadFromDisk();
                if(employee_info.firstName != NULL) {
                    sendData.status = READ_OK;
                    strcpy(myMessage,"Read OK");
                    myMessage[strlen(myMessage)] = '\0';
                    sendData.eData = employee_info;
                    strncpy(sendData.message, myMessage, strlen(myMessage));
                    sendto(newSockfd, &sendData, sizeof(struct SEND_DATA_STRUC), 0, 
                        (struct sockaddr*)&clientAddr, sizeof(clientAddr));  
                }
            }

            //Logout Process
            if(strncmp(message, "LOGOUT", 6) == 0) {
                sendMesage.status = SESSION_CLOSED;
                strcpy(myMessage,"Session closed. Re-authenticate to use service again!! \n");
                myMessage[strlen(myMessage)] = '\0';
                strncpy(sendMesage.message, myMessage, strlen(myMessage));
                sendto(newSockfd, &sendMesage, sizeof(struct MESSAGE_STRUC), 0, 
                    (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                authed = 0;
                user = 0;
                auth = 0;
            }
        }
    }
}