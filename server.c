#include "server.h"

void child_killer(int signo, siginfo_t* sinfo, void* context) {
    // If the signal received is SIGUSR1 then kill the child who sent it
    if (signo == SIGUSR1){
        waitpid(sinfo->si_pid, NULL, 0);
        kill(sinfo->si_pid, SIGKILL);
    }
}

int main(int argc, char** argv) {
    // Initialise the signal handler
    struct sigaction sig;
    memset(&sig, 0, sizeof(struct sigaction));
    sig.sa_sigaction = child_killer; //SETS Handler
    sig.sa_flags = SA_SIGINFO;
    if(sigaction(SIGUSR1, &sig, NULL) == -1) {
        perror("Sigaction failed");
        return 1;
    }
    // Setup global pipe
    if (mkfifo(GLOBAL_PIPE_NAME, 0777) == -1){
        if (errno != EEXIST){
            printf("Could not create global pipe '%s'.\n", GLOBAL_PIPE_NAME);
            perror("");
            return 1;
        }
    }

    // Open the global pipe for reading
    int gevent_RD = open(GLOBAL_PIPE_NAME, O_RDONLY); 
    if (gevent_RD == -1){
        printf("Could not open global pipe.\n");
        return 1;
    }

    // Initialise the buffer for the global process to read data from the pipe into 
    char buffer[MSG_SIZE];
    void* buffer_ptr = &buffer;

    // Now stay in an infinite loop and read

    while (1){
        // Try read any messages that come through
        int amount = read(gevent_RD, buffer, MSG_SIZE);
        if (amount > 0){
            u_int16_t type = *((u_int16_t*) buffer_ptr);

            if (type == CONNECT){
                // Connect and spawn the client handler
                char identifier[256];
                strcpy(identifier, (char*) buffer_ptr + 2);
                char domain[256];
                strcpy(domain, (char*) buffer_ptr + 258);
                connect(identifier, domain);
                continue;
            } else if (type == TERMINATE){
                // Terminate the global process and clean up gevent
                close(gevent_RD);
                remove(GLOBAL_PIPE_NAME);
                exit(0);
            }
        }
        
    }
}

void connect(char *identifier, char* domain){
    // Will set errno if directory doesnt exist
    errno = 0;
    DIR* dir = opendir(domain);
    char path[256];
    strcpy(path, domain);
    if (ENOENT == errno){
        // Directory does not exist, create it
        // Loop through each token and try mkdir
        path[0] = '\0';
        for (char *p = strtok(domain, "/"); p!= NULL; p = strtok(NULL, "/")){
            strcat(path, p);
            strcat(path, "/");
            mkdir(path, 0777);
        }
    } else{
        strcat(path, "/");
    }
    closedir(dir);
    // Directory now exists
    // Create the FIFO
    // Add an additional 3 chars for _RD or _WR
    char read_pipe[512 + 3];
    char write_pipe[512 + 3];

    strcpy(read_pipe, path);
    strcpy(write_pipe, path);

    strcat(read_pipe, identifier);
    strcat(write_pipe, identifier);

    strcat(read_pipe, "_RD");
    strcat(write_pipe, "_WR");

    // Create the named pipe
    if (mkfifo(read_pipe, 0777) == -1){
        printf("Creation of %s failed.\n", read_pipe);
        return;
    }
    if (mkfifo(write_pipe, 0777) == -1){
        printf("Creation of %s failed.\n", write_pipe);
        return;
    }
    int result = fork();
    if (result == 0){
        // Pass off the rest of the program to client handler
        client_handler(read_pipe, write_pipe, domain, identifier);
    }
    // It is global so return back to the while loop and wait for more connects
}

void client_handler(char *read_pipe, char *write_pipe, char *domain, char *identifier){
    // The handler read's from the client's write pipe and writes ping to client's read pipe
    int client_reader = open(write_pipe, O_RDONLY);

    char buffer[MSG_SIZE];
    void* buffer_ptr = &buffer;

    int client_list_size = 0;
    struct client *client_list = malloc(0);

    // For ping pong
    // Start time and current time will help determine when to send ping, and if a pong was not received
    char ping_buffer[MSG_SIZE];
    memset(ping_buffer, '\0' , MSG_SIZE);
    *((u_int16_t*) &ping_buffer) = PING;
    time_t start_time;
    time(&start_time);
    time_t current_time;
    // Flag to say whether or not the handler is waiting for a ping
    int waiting_for_pong = 0;

    // Initialise client_reader poll so that it can only be read from while there is something to read
    struct pollfd npipes[1];
    npipes[0].fd = client_reader;
    npipes[0].events = POLLIN;

    int amount_read;
    int messages_to_send = 0;

    while (1){
        memset(buffer, '\0' , MSG_SIZE);

        // Update current time
        time(&current_time);
        if (difftime(current_time, start_time) >= 15){
            if (waiting_for_pong == 0){
                // Open and send a ping to the client
                int client_writer = open(read_pipe, O_WRONLY);
                write(client_writer, ping_buffer, MSG_SIZE);
                close(client_writer);
                // Set flag
                waiting_for_pong = 1;
                // Update start time to current time so we essentially reset the clock to 0.
                time(&start_time);
            }
        } else if (difftime(current_time, start_time) >= 3 && waiting_for_pong){
            // After a resonable amount of time has passed (3seconds) and the client has not responded
            // with a pong, then perform a disconnect because client is dead.
            close(client_reader);
            disconnect(read_pipe, write_pipe, &client_list, client_list_size);
        }

        //Try read a message sent from client, using a 0.1second timeout
        int ret = poll(npipes, 1, 100);
        amount_read = ret;
        if (npipes[0].revents & POLLIN && amount_read > 0){
            // There is something to read
            amount_read = read(client_reader, buffer, MSG_SIZE);
        }

        if (amount_read > 0){
            u_int16_t type = *((u_int16_t*) buffer_ptr);
            // Based on the given type, try to perform the command
            // If the type is invalid then the command is ignored
            if (type == SAY){
                if (!is_valid_message((char*) buffer_ptr + 2, 1790)){
                    continue;
                }
                relay_message((char*) buffer_ptr + 2, domain, read_pipe, identifier, &client_list, &client_list_size, &messages_to_send, RECEIVE);
            } else if (type == SAYCONT){
                if (!is_valid_message((char*) buffer_ptr + 2, 1789)){
                    continue;
                }
                relay_message((char*) buffer_ptr + 2, domain, read_pipe, identifier, &client_list, &client_list_size, &messages_to_send, RECVCONT);
            } else if (type == DISCONNECT){
                close(client_reader);
                disconnect(read_pipe, write_pipe, &client_list, client_list_size);
            } else if (type == PONG){
                if (waiting_for_pong){
                    waiting_for_pong = 0;
                }
            }
        }
        // If there are now pending messages, try send them to the other clients
        if (messages_to_send > 0){
            flush_all_messages(&client_list, &client_list_size, identifier, &messages_to_send);
        }
    }
}

void relay_message(char *message, char *domain ,char *self_pipe, char *self_name, struct client **client_list, int *client_size, int *messages_left, int message_type){
    // Read all files in the directory
    char current_file[256];
    char full_path[512];
    DIR *directory = opendir(domain);
    struct dirent *dir;
    if (directory){
        while ((dir = readdir(directory)) != NULL){
            strcpy(current_file, dir->d_name);
            int file_name_length = strlen(current_file);
            
            // Check if it's the same pipe, if it is then skip it
            if (strlen(self_name) == strlen(current_file) - 3){
                if (strncmp(self_name, current_file, strlen(self_name)) == 0){
                    continue;
                }
            }
            // We know that the pipe must contain _WR or _RD, so it must be at least 4 chars
            if (file_name_length < 4){
                continue;
            }
            const char *last_three = &current_file[file_name_length - 3];
            if (strncmp(last_three, "_RD", 3) == 0){
                // It is a _RD pipe, and it is not its ownpipe.
                
                strcpy(full_path, domain);
                strcat(full_path, "/");
                strcat(full_path, current_file);

                // Check if the client struct of the pipe exists yet
                int found = 0;
                struct client *c;
                for (int i = 0; i < *client_size; i++){
                    c = *client_list + i;
                    if (strcmp(full_path, c->pipe_RD) == 0){
                        found = 1;
                        break;
                        // Then the client exists already in memory, add on message to queue    
                    }
                }
                if (!found){
                    // Create a new client struct
                    (*client_size)++;
                    *client_list = realloc(*client_list ,sizeof(struct client) * *client_size);
                    c = *client_list + *client_size - 1;
                    c->message_count = 0;
                    c->messages = malloc(0);
                    c->message_type = malloc(0);
                    strcpy(c->pipe_RD, full_path);
                }
                // Allocate more memory for the new message to be stored, and copy the message over
                // Update the relevant values such as message count
                c->messages = realloc(c->messages, sizeof(char *) * (c->message_count + 1));
                c->message_type = realloc(c->message_type, sizeof(int) * (c->message_count + 1));
                c->messages[c->message_count] = malloc(sizeof(char) * 2046);
                memcpy(c->messages[c->message_count], message, 2046);
                c->message_type[c->message_count] = message_type;
                c->message_count++;
                (*messages_left)++;
                c->fd = -1;  
            }
        }
    }
    closedir(directory);
}

void flush_all_messages(struct client **client_list, int *client_size, char *self_name, int *messages_left){
    // Initialise the set of pipes
    fd_set currently_open_pipes;
    FD_ZERO(&currently_open_pipes);

    // Loop through all structs and open for write so we can pass the fds to select
    for (int i = 0; i < *client_size; i++){
        struct client *c = *client_list + i;
        int fd = open(c->pipe_RD, O_WRONLY);
        if (fd < 0){
            if (errno == ENOENT){
                // Pipe no longer exists anymore, so remove the struct and free its relevant data
                for (int j = 0; j < c->message_count; j++){
                    free(c->messages[j]);
                }
                free(c->messages);
                if (*client_size - i > 1){
                    memmove(c, c + 1, sizeof(struct client) * (*client_size - i - 1));
                }
                (*client_size)--;
                i--;
                *client_list = realloc(*client_list, sizeof(struct client) * *client_size);
                continue;
            } else {
                perror("pipe_RD open failed");
            }
        } else {
            c->fd = fd;
            FD_SET(fd, &currently_open_pipes);
        }
    }

    // timeout struct
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // Run select with 0 second delay in order to see which pipes are currently available to write to
    if (select(FD_SETSIZE, NULL, &currently_open_pipes, NULL, &tv) < 0){
        perror("Select error");
    }
    // Loop through all the available pipes
    for (int i = 0; i < FD_SETSIZE; i++){
        // The pipe is available
        if (FD_ISSET(i, &currently_open_pipes)){
            char buffer[MSG_SIZE];
            void *buffer_ptr = &buffer;
            // Find the client associated with this available pipe
            for (int j = 0; j < *client_size; j++){
                struct client *c = *client_list + j;
                if (c->fd == i){
                    // This client is available to be written to
                    int k = 0;
                    for (; k < c->message_count; k++){
                        memset(buffer, '\0', MSG_SIZE);
                        *((u_int16_t*) buffer_ptr) = c->message_type[k];
                        strcpy((char*) buffer_ptr + 2, self_name);
                        memcpy(buffer_ptr + 258, c->messages[k], 1790);

                        // Add termination byte if this is the last recvcont message in the stream
                        if (c->message_type[k] == RECVCONT){
                            if (c->messages[k][2045] == (char) 255){
                                buffer[MSG_SIZE - 1] = (char) 255;
                            }
                        }

                        int written = write(c->fd, buffer, MSG_SIZE);
                        if (written < 0){
                            perror("Write error");
                            break;
                        }
                        free(c->messages[k]);
                        (*messages_left)--;
                    }
                    // If there are still unsent messages, then shift them down to the start of the array
                    if (c->message_count - k > 1){
                        memmove(c->messages[0], c->messages[k + 1], sizeof(char*) * (c->message_count - k - 1));
                        memmove(&c->message_type[0], &c->message_type[k + 1], sizeof(int) * (c->message_count - k - 1));
                    }
                    c->message_count -= (k + 1);
                    if (c->message_count < 0){
                        c->message_count = 0;
                    }
                    // Resize the message and type array
                    c->messages = realloc(c->messages, sizeof(char*) * c->message_count);
                    c->message_type = realloc(c->message_type, sizeof(int) * (c->message_count));
                    close(c->fd);
                    c->fd = -1;
                }
            }
        }
    }
}

int is_valid_message(char* msg, int size){
    // Check if this is a valid string
    for (int i = 0; i < size; i++){
        if (msg[i] == '\0'){
            // A null terminator exists so it is a valid string
            return 1;
        }
    }
    // No null terminator could be found, return 0
    return 0;
}

void disconnect(char *read_pipe, char *write_pipe, struct client **client_list, int size){
    // Delete the pipes
    remove(read_pipe);
    remove(write_pipe);
    // Free everything
    for (int i = 0; i < size; i++){
        struct client *c = *client_list + i;
        for (int j = 0; j < c->message_count; j++){
            free(c->messages[j]);
            
        }
        free(c->message_type);
        free(c->messages);
    }
    free(*client_list);
    // Send signal to parent
    kill(getppid(), SIGUSR1);
    // Terminate
    exit(0);
}