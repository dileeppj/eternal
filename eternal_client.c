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

#define SERVER_QUEUE_NAME   "/eternal-server"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 256
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 10

time_t now;

mqd_t qd_server, qd_client;   // queue descriptors
char client_queue_name [64];

void sig_handler(int signum) {
    printf("\n[-] Exiting Application\n");
    mq_close(qd_server);
    mq_close(qd_client);
    mq_unlink(client_queue_name);
    exit(0);
}

int main (int argc, char **argv)
{

    if(strcmp(argv[1],"CONNECT") || argc != 3 ){
      perror("[-] Error : Pass with arguments. eg. ./eternal_client CONNECT <client_id> ");
      exit(1);
    }
    sprintf (client_queue_name, "/eternal-client-%s", argv[2]);

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    // SIGNAL INTERRUPTION
    signal(SIGINT, sig_handler);  // CTRL+C


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

        // time stamp TODO
        // char buf[16];
        // snprintf(buf, 16, "%lu", time(NULL));
        // strcat(temp_buf,buf);

        //Send data - [position,velocity] - data in temp_buf
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
