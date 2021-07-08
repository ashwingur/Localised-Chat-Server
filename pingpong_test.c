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

    sleep(1);
    errno = 0;
    int ashwin_read = open("chatroom/Ashwin_RD", O_RDONLY | O_NONBLOCK);
    int ashwin_write = open("chatroom/Ashwin_WR", O_WRONLY | O_NONBLOCK);

    if (ashwin_read < 0 || ashwin_write < 0){
        perror("Ashwin could not open pipe");
    }


    while (1){
        int amount_read = read(ashwin_read, buffer, MSG_SIZE);
        if (amount_read > 0){
            printf("%d\n",*((u_int16_t*) buffer_ptr));
            // Received a ping so send a pong back
            memset(buffer, '\0', 2048);
            *((u_int16_t*) buffer_ptr) = PONG;
            write(ashwin_write, buffer, MSG_SIZE);
            break;
        }
        sleep(1);
    }
    // Do not respond to the next ping
    sleep(20);

    ashwin_read = open("chatroom/Ashwin_RD", O_RDONLY | O_NONBLOCK);
    ashwin_write = open("chatroom/Ashwin_WR", O_WRONLY | O_NONBLOCK);

    if (ashwin_read >= 0 || ashwin_write >= 0){
        printf("Ashwin's pipes in chatroom did not disconnect!\n");
    }
    
    *((u_int16_t*) buffer_ptr) = TERMINATE;
    write(gevent_WR, buffer, MSG_SIZE);

    close(gevent_WR);
    close(ashwin_read);
    close(ashwin_write);
    rmdir("chatroom");
    return 0;
}