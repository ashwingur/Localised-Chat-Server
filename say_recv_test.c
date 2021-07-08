#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>



int main(int argc, char** argv) {
    sleep(2);
    // Open the global pipe for reading
    int gevent_WR = open(GLOBAL_PIPE_NAME, O_WRONLY); 
    if (gevent_WR == -1){
        printf("Could not open global pipe.\n");
        return 1;
    }

    char buffer1[2048];
    char buffer2[2048];
    memset(buffer1, '\0', 2048);
    memset(buffer2, '\0', 2048);
    void* buffer1_ptr = &buffer1;
    void* buffer2_ptr = &buffer2;
    *((u_int16_t*) buffer1_ptr) = CONNECT;
    *((u_int16_t*) buffer2_ptr) = CONNECT;
    
    strcpy((char*) buffer1_ptr + 2, "Ashwin");
    strcpy((char*) buffer1_ptr + 258, "chatroom");

    strcpy((char*) buffer2_ptr + 2, "Bob");
    strcpy((char*) buffer2_ptr + 258, "chatroom");

    write(gevent_WR, buffer1, MSG_SIZE);
    write(gevent_WR, buffer2, MSG_SIZE);
    sleep(1);
    errno = 0;
    int ashwin_read = open("chatroom/Ashwin_RD", O_RDONLY | O_NONBLOCK);
    int ashwin_write = open("chatroom/Ashwin_WR", O_WRONLY | O_NONBLOCK);

    if (ashwin_read < 0 || ashwin_write < 0){
        perror("Ashwin could not open pipe");
    }

    int bob_read = open("chatroom/Bob_RD", O_RDONLY | O_NONBLOCK);
    int bob_write = open("chatroom/Bob_WR", O_WRONLY | O_NONBLOCK);

    if (bob_read < 0 || bob_write < 0){
        perror("Bob could not open pipe");
    }

    *((u_int16_t*) buffer1_ptr) = SAY;
    strcpy((char*) buffer1_ptr + 2, "Hello, my name is Ashwin. What is your name?");
    write(ashwin_write, buffer1, MSG_SIZE);

    memset(buffer2, '\0', MSG_SIZE);
    while (1){
        int amount_read = read(bob_read, buffer2, MSG_SIZE);
        if (amount_read > 0){
            break;
        }
        sleep(1);
    }

    printf("%d|%s|%s\n",*((u_int16_t*) buffer2_ptr), (char*) buffer2_ptr + 2, (char*) buffer2_ptr + 258);

    *((u_int16_t*) buffer2_ptr) = SAY;
    strcpy((char*) buffer2_ptr + 2, "Hey Ashwin! I am Bob and it is nice to meet you. I gotta go now, cy@.");
    write(bob_write, buffer2, MSG_SIZE);

    memset(buffer1, '\0', MSG_SIZE);
    while (1){
        int amount_read = read(ashwin_read, buffer1, MSG_SIZE);
        if (amount_read > 0){
            break;
        }
        sleep(1);
    }

    printf("%d|%s|%s\n",*((u_int16_t*) buffer1_ptr), (char*) buffer1_ptr + 2, (char*) buffer1_ptr + 258);

    memset(buffer1, '\0', MSG_SIZE);
    *((u_int16_t*) buffer1_ptr) = SAY;
    strcpy((char*) buffer1_ptr + 2, "No worries Bob, it was nice meeting you. Goodbye.");
    write(ashwin_write, buffer1, MSG_SIZE);

    memset(buffer2, '\0', MSG_SIZE);
    while (1){
        int amount_read = read(bob_read, buffer2, MSG_SIZE);
        if (amount_read > 0){
            break;
        }
        sleep(1);
    }

    printf("%d|%s|%s\n",*((u_int16_t*) buffer2_ptr), (char*) buffer2_ptr + 2, (char*) buffer2_ptr + 258);

    *((u_int16_t*) buffer1_ptr) = DISCONNECT;
    *((u_int16_t*) buffer2_ptr) = DISCONNECT;
    write(ashwin_write, buffer1, MSG_SIZE);
    write(bob_write, buffer2, MSG_SIZE);
    
    sleep(2);
    *((u_int16_t*) buffer1_ptr) = TERMINATE;
    write(gevent_WR, buffer1, MSG_SIZE);

    close(gevent_WR);
    close(ashwin_read);
    close(ashwin_write);
    close(bob_read);
    close(bob_write);
    rmdir("chatroom");
    
    return 0;
}

