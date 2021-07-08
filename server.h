#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>
#include <poll.h>

// Setting up macros
#define GLOBAL_PIPE_NAME "gevent"
#define MSG_SIZE 2048

// Protocols
#define CONNECT 0
#define SAY 1
#define SAYCONT 2
#define RECEIVE 3
#define RECVCONT 4
#define PING 5
#define PONG 6
#define DISCONNECT 7
// Terminate is to terminate the global process (for testcases)
#define TERMINATE 8

// This struct represents all other clients in the domain (except the client handler's client)
// It contains the list of messages to send to other clients, in order.
struct client{
    char pipe_RD[512];
    char **messages;
    int *message_type;
    int message_count;
    int fd;
};

// Based on the given identifier and domain, creates the _RD and _WR pipes, and then forks the program
// The global process exits out of connect, while the newly forked child calls client_handler
void connect(char *identifier, char *domain);

// After the global process forks, the child calls this function, in which it stays for the rest of its
// lifetime. All client handler calls are dealt with in this
void client_handler(char *read_pipe, char *write_pipe, char *domain, char *identifier);

// Adds the received message into all the client structs (creates any new client structs if a new client has joined)
void relay_message(char *message, char *domain ,char *self_pipe, char *self_name, struct client **client_list, int *client_size, int *messages_left, int message_type);

// Loop through all the client structs and try send the messages through their pipes if they are available for writing
void flush_all_messages(struct client **client_list, int *client_size, char *self_name, int *messages_left);

// Removes the client's pipes, sends a signal to the global process and then exits
void disconnect(char *read_pipe, char *write_pipe, struct client **client_list, int size);

// The signal handler of the global process to clean up the child
void child_killer(int signo, siginfo_t* sinfo, void* context);

// Used when a say/saycont is received to check if the message is indeed a valid string
int is_valid_message(char* msg, int size);
#endif
