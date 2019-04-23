#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
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


mqd_t qd_server, qd_client;   // queue descriptors
int sockfd, newSocket;

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
    printf("[-] Received SIGINT. Exiting Application\n");
    mq_close(qd_server);
    mq_close(qd_client);
    mq_unlink(SERVER_QUEUE_NAME);
    close(newSocket);
    close(sockfd);
    exit(0);
}

int main (int argc, char **argv)
{
    s_data msg;
    // ********************************** SOCKET *******************************
    struct sockaddr_in serverAddr, newAddr;
    socklen_t addr_size = sizeof(serverAddr);

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
  		printf("[-] Error in connection.\n");
  		exit(1);
  	}
  	printf("[+] Server Socket is created.\n");

  	memset(&serverAddr, '\0', sizeof(serverAddr));  //memset() is used to fill a block of memory with a particular value.
  	serverAddr.sin_family = AF_INET;
  	serverAddr.sin_port = htons(PORT);
  	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  	if((bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) < 0){
  		printf("[-] Error in binding.\n");
  		exit(1);
  	}
  	printf("[+] Bind to port %d\n", PORT);

  	if(listen(sockfd, 10) == 0){
  		printf("[+] Listening....\n");
  	}else{
  		printf("[-] Error in binding.\n");
  	}
    // ********************************** MESSAGE QUEUE ************************

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    printf ("[.] Server : Waiting for Client Messages\n");
    // STARTING THE SERVER NODE
    if ((qd_server = mq_open (SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror ("[-] Server : mq_open (server)");
        exit (1);
    }
    char in_buffer [MSG_BUFFER_SIZE];
    char out_buffer [MSG_BUFFER_SIZE];
    char delim[] = ",";
    char temp[MSG_BUFFER_SIZE] = {0};

    // SIGNAL INTERRUPTION
    signal(SIGINT, sig_handler);  // CTRL+C

    // ********************************** SOCKET *******************************
    while(1){   // THIS WILL BE OUR OUTER LOOP - WHEN A NEW SOCKET CONNETION IS ESTABLISHED HERE i.e,NEW CLIENT
		newSocket = accept(sockfd, (struct sockaddr*)&newAddr, &addr_size);
		if(newSocket < 0){
			exit(1);
		}
		printf("[.] Connection accepted from Socket Addr %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
    // HERE WE HAVE TO MAKE NEW THREAD AND MESSAGE QUEUE FOR THE CLIENT
    // FOR THAT WE REQUIRE CLIENT ID, IT IS PASSED FROM THE CLIENT FOR INIT. THE CONNECTION
    char buffer[1024];
    // send(clientSocket, buffer, strlen(buffer), 0);
    recv(newSocket, buffer, 1024, 0);
    printf("[.] Client ID (str) : %s\n", buffer);
    // The message is received in the var. buffer
    int client_id = atoi(buffer);
    printf("[.] Client ID (int) : %d\n", client_id);
    // Therefore the name of new message queue is /eternal-client-<client_id>
    //Send the new client ID to client
    memset(&buffer, '\0', sizeof(buffer));
    sprintf(buffer, "eternal-client-%d",client_id);
    send(newSocket, buffer, strlen(buffer), 0);

    // So we have sent the name of message queue to the client
    // MAKE NEW THREAD
    // MAKE NEW MESSAGE QUEUE for the client
    char client_queue_name [64];
    sprintf(client_queue_name, "/%s", buffer);
    if ((qd_client = mq_open (client_queue_name, O_WRONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
      perror ("[-] Client : mq_open (client)");
      exit (1);
    }
    // STARTING THE CLIENT NODE
    // Ok.. so we are sending the Connect request of the client to the Servers message queue
    // sprintf(in_buffer, "/", buffer);
    while (1) {
        // Receiving the client message queue name (in in_buffer) to communicate
        if (mq_receive (qd_server, in_buffer, MSG_BUFFER_SIZE, NULL) == -1) {
            perror ("[-] Server : mq_receive");
            exit (1);
        }

        // Open lient message queue
        if ((qd_client = mq_open (in_buffer, O_WRONLY)) == 1) {
            perror ("[-] Server : Not able to open client queue");
            continue;
        }
        printf("[+] Connection established to : %s\n", in_buffer);

        // Receive data
        if (mq_receive (qd_server, in_buffer, MSG_BUFFER_SIZE, NULL) == -1) {
            perror ("[-] Server : Message error");
            exit (1);
        }

        //Print the message on screen
        printf ("[+] Server : Message received from client.\n");
        printf ("[+] Message :  %s",in_buffer);
        //TO EXTRACT THE DATA {POS & VEL}
        char *ptr = strtok(in_buffer, delim);
        memset(&temp, 0, sizeof(temp));
        while (ptr != NULL)
        {
          strcat(temp,ptr);
          strcat(temp,"-");
          ptr = strtok(NULL, delim);
        }
        //Modification on the data got from client
        sprintf (out_buffer, "Modified : %s", temp);
        printf("[.] Modified Message : %s\n", out_buffer);

        // Sending modified message to client
        if (mq_send (qd_client, out_buffer, strlen (out_buffer) + 1, 0) == -1) {
            perror ("[-] Server : Not able to send message to client");
            continue;
        }

        printf ("[-] Server : response sent to client.\n");
    }
	}
}
