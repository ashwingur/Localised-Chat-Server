#include "server.h"

int main(int argc, char** argv) {
    sleep(2);
    // Open the global pipe for reading
    int gevent_WR = open(GLOBAL_PIPE_NAME, O_WRONLY); 
    if (gevent_WR == -1){
        printf("Could not open global pipe.\n");
        return 1;
    }

    char buffer[2048];
    memset(buffer, '\0', 2048);
    void* buffer_ptr = &buffer;
    *((u_int16_t*) buffer_ptr) = CONNECT;
    
    strcpy((char*) buffer_ptr + 2, "Ashwin");
    strcpy((char*) buffer_ptr + 258, "chatroom");
    write(gevent_WR, buffer, MSG_SIZE);

    strcpy((char*) buffer_ptr + 2, "Bob");
    strcpy((char*) buffer_ptr + 258, "chatroom");
    write(gevent_WR, buffer, MSG_SIZE);

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
   
    *((u_int16_t*) buffer_ptr) = SAYCONT;
    strcpy((char*) buffer_ptr + 2, "Good day to you sir. My name is Ashwin and the message i'm about to send");
    write(ashwin_write, buffer, MSG_SIZE);

    memset(buffer, '\0', MSG_SIZE);
    *((u_int16_t*) buffer_ptr) = SAYCONT;
    strcpy((char*) buffer_ptr + 2, "is going to be big, so I am sending it in chunks");
    write(ashwin_write, buffer, MSG_SIZE);

    memset(buffer, '\0', MSG_SIZE);
    *((u_int16_t*) buffer_ptr) = SAYCONT;
    strcpy((char*) buffer_ptr + 2, "Which you shall receive in order! (btw the termination byte is in this chunk)");
    buffer[MSG_SIZE - 1] = (char) 255;
    write(ashwin_write, buffer, MSG_SIZE);

    while (1){
        int amount_read = read(bob_read, buffer, MSG_SIZE);
        if (amount_read > 0){
            printf("%d|%s|%s\n",*((u_int16_t*) buffer_ptr), (char*) buffer_ptr + 2, (char*) buffer_ptr + 258);
            if (buffer[MSG_SIZE - 1] == (char) 255){
                break;
            }
        }
        sleep(1);
    }

    *((u_int16_t*) buffer_ptr) = DISCONNECT;
    write(ashwin_write, buffer, MSG_SIZE);
    write(bob_write, buffer, MSG_SIZE);
    
    sleep(2);
    *((u_int16_t*) buffer_ptr) = TERMINATE;
    write(gevent_WR, buffer, MSG_SIZE);

    close(gevent_WR);
    close(ashwin_read);
    close(ashwin_write);
    close(bob_read);
    close(bob_write);
    rmdir("chatroom");
    return 0;
}