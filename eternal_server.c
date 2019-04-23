#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define SERVER_QUEUE_NAME   "/eternal-server"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 256
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 10

mqd_t qd_server, qd_client;   // queue descriptors

void sig_handler(int signum) {
    printf("[-] Received SIGINT. Exiting Application\n");
    mq_close(qd_server);
    mq_close(qd_client);
    mq_unlink(SERVER_QUEUE_NAME);
    exit(0);
}

int main (int argc, char **argv)
{

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
