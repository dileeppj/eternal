#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_QUEUE_NAME   "/eternal-server"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 512
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 10
#define DATE_LEN 100

time_t now;

mqd_t qd_server, qd_client;   // queue descriptors
char client_queue_name [64];
int clientSocket;

// ********************************* DATA FORMAT *******************************

typedef struct struct_data {
  int pos_x;
  int pos_y;
  int pos_z;
  int vel_x;
  char date[DATE_LEN];
} s_data;

// *************************************************************************

void sig_handler(int signum) {
    printf("\n[-] Exiting Application\n");
    mq_close(qd_server);
    mq_close(qd_client);
    mq_unlink(client_queue_name);
    close(clientSocket);
    exit(0);
}
//Rough code to copy data from terminal to struct
// void copy(i,ptr){
//   switch (i) {
//     case 1: msg.pos_x = ptr;
//     case 2: msg.pos_y = ptr;
//     case 3: msg.pos_z = ptr;
//     case 4: msg.vel_x = ptr;
//   }

int main (int argc, char **argv)
{
    if(strcmp(argv[1],"CONNECT") || argc != 3 ){
      perror("[-] Error : Pass with arguments. eg. ./eternal_client CONNECT <client_id> ");
      exit(1);
    }
    s_data msg;
    time_t clock;
    // Check if there is an entry for a client with same client id with the name of client message queue (as it is same as client_id
    // If any of the client has same client id, break the program and ask for new client id
    // If there is no client with similar client id, then make a new thread and a message queue for that thread to communicate with server

    // ********************************** SOCKET *******************************

    struct sockaddr_in serverAddr;

    if((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0){
      printf("[-] Error in connection.\n");
      exit(1);
    }
    printf("[+] Client Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
      printf("[-] Error in connection.\n");
      exit(1);
    }
    printf("[+] Connected to Server.\n");

    // TO DO below
    // Now as I'm connected to the server, i need to send the client_id (argv[2]) to the server
    // to create a message queue and a thred (N_Imp) and the server sends me the name of the
    // message queue to connect with.
    char buffer[1024];
    sprintf(buffer, "%s",argv[2]);
    printf("[.] Client ID : %s\n", buffer);
    send(clientSocket, buffer, strlen(buffer), 0);
    // The message argv[1] i.e the client_id, is sent to the server
    // The server will send the name of message queue, receive it
    memset(&buffer, '\0', sizeof(buffer));
    recv(clientSocket, buffer, 1024, 0);
    // The var. buffer has the name of message server to be created by server for the client
    // sprintf (client_queue_name, "/eternal-client-%s", argv[2]);
    sprintf(client_queue_name, "/%s", buffer);
    printf("[.] Client Queue Name : %s\n", client_queue_name);

    // ********************************** MESSAGE QUEUE ************************
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    // SIGNAL INTERRUPTION
    signal(SIGINT, sig_handler);  // CTRL+C

    // ********************************** MESSAGE QUEUE ************************

    // STARTING THE CLIENT NODE
    if ((qd_client = mq_open (client_queue_name, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror ("[-] Client : mq_open (client)");
        exit (1);
    }
    // STARTING THE SERVER NODE
    if ((qd_server = mq_open (SERVER_QUEUE_NAME, O_WRONLY)) == -1) {
        perror ("[-] Client : mq_open (server)");
        exit (1);
    }


    char in_buffer [MSG_BUFFER_SIZE];
    char temp_buf[MAX_MSG_SIZE];
    char delim[] = ",";
    // char temp[MSG_BUFFER_SIZE] = {0};

    // printf ("Data to be sent [Position & velocity] : ");
    printf ("[=] Client [pos,vel] : ");
    while (fgets (temp_buf, MAX_MSG_SIZE, stdin)) {
        // Checking for STOP
        if(strcmp(temp_buf,"STOP\n") == 0){
          sig_handler(2);
        }

        // send queue name to server
        if (mq_send (qd_server, client_queue_name, strlen (client_queue_name) + 1, 0) == -1) {
            perror ("[-] Client : Not able to send message to server");
            continue;
        }

        // Timestamp
        time(&clock);
        snprintf(msg.date, sizeof(msg.date), "%.24s", ctime(&clock));
        // Getting value from terminal - temp_buf & extract it down.
        //TO EXTRACT THE DATA {POS & VEL}
        // char *ptr = strtok(temp_buf, delim);
        // int i=0;
        // // memset(&temp, 0, sizeof(temp));
      	// while (ptr != NULL){
      	// 	// strcat(temp,ptr);
        //   // strcat(temp,"-");
      	// 	ptr = strtok(NULL, delim);
        //   copy(i,ptr);
        //   i++;
        // }

        //Send data - [position,velocity] - data in temp_buf
        // if (mq_send (qd_server, (const char*)&msg, strlen (msg) + 1, 0) == -1) {
        if (mq_send (qd_server, temp_buf, strlen (temp_buf) + 1, 0) == -1) {
            perror ("[-] Client : Not able to send message to server");
            continue;
        }

        // receive modified response to the data from server - data goes to in_buffer
        if (mq_receive (qd_client, in_buffer, MSG_BUFFER_SIZE, NULL) == -1) {
            perror ("[-] Server : Message error");
            sig_handler(2);
            // exit (1);
        }

        //Print the message on screen
        printf ("[.] Server : message received.\n");
        printf("[.] Message : %s\n", in_buffer);
        printf ("[=] Client [pos,vel] : ");
    }

    exit (0);
}
