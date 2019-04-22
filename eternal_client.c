#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
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

int main (int argc, char **argv)
{
    char client_queue_name [64];
    mqd_t qd_server, qd_client;   // queue descriptors


    // create the client queue for receiving messages from server
    // sprintf (client_queue_name, "/eternal-client-%d", getpid ());
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

    if ((qd_client = mq_open (client_queue_name, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror ("Client: mq_open (client)");
        exit (1);
    }

    if ((qd_server = mq_open (SERVER_QUEUE_NAME, O_WRONLY)) == -1) {
        perror ("Client: mq_open (server)");
        exit (1);
    }

    char in_buffer [MSG_BUFFER_SIZE];

    printf ("Data to be sent [Position & velocity] : ");

    char temp_buf[MAX_MSG_SIZE];


    while (fgets (temp_buf, MAX_MSG_SIZE, stdin)) {

        // send message to server
        if (mq_send (qd_server, client_queue_name, strlen (client_queue_name) + 1, 0) == -1) {
            perror ("Client: Not able to send message to server");
            continue;
        }

        // time stamp
        // char buf[16];
        // snprintf(buf, 16, "%lu", time(NULL));
        // strcat(temp_buf,buf);

        //Send data - [position,velocity]
        if (mq_send (qd_server, temp_buf, strlen (temp_buf) + 1, 0) == -1) {
            perror ("Client: Not able to send message to server");
            continue;
        }

        // receive response from server

        if (mq_receive (qd_client, in_buffer, MSG_BUFFER_SIZE, NULL) == -1) {
            perror ("Client: mq_receive");
            exit (1);
        }
        // display token received from server
        printf ("Server: %s\n\n", in_buffer);

        printf ("Data to be sent [Position & velocity] : ");
    }


    if (mq_close (qd_client) == -1) {
        perror ("Client: mq_close");
        exit (1);
    }

    if (mq_unlink (client_queue_name) == -1) {
        perror ("Client: mq_unlink");
        exit (1);
    }
    printf ("Client: bye\n");

    exit (0);
}