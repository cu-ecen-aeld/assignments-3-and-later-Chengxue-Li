#include "aesdsocket.h"
#include <limits.h>

static void signal_handler(int signo) {
    signal_flag = signo;
}

void close_all() {
    if (fd != -1) {
        close(fd);
    }
    if (sockfd != -1) {
        close(sockfd);
    }
    if (servinfo != NULL) {
        freeaddrinfo(servinfo);
    }
    closelog();
}

void *timer_routine(void *arg) {
    char buffer[80];
    while (1) {
        sleep(10); // sleep for 10 seconds
        time_t now;
        time(&now);
        struct tm *local = localtime(&now);
        strftime(buffer, 80, "timestamp:%a, %d %b %Y %T %z\n", local);
        pthread_mutex_lock(&mutex);
        write(fd, buffer, strlen(buffer));
        pthread_mutex_unlock(&mutex);
        
    }

    return 0;
}

void *routine(void *arg) {
    struct thread_data *new_thread_data = (struct thread_data *)arg;
    int client_sockfd = new_thread_data->client_sockfd;
    char *client_ip = new_thread_data->client_ip;

    char buffer[10];
    char file_buffer[10];

    int lock_flag = 0;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(client_sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            syslog(LOG_ERR, "Error receiving data from client");
            close(client_sockfd);
            new_thread_data->thread_node.thread_complete = 1;
            return new_thread_data;
        }
        if (bytes_received == 0) {
            syslog(LOG_INFO, "Closed connection from %s\n", client_ip);
            close(client_sockfd);
            new_thread_data->thread_node.thread_complete = 1;
            return new_thread_data;
        }
        if (lock_flag == 0) {
            pthread_mutex_lock(&mutex);
            lock_flag = 1;
        }
        
        if (write(fd, buffer, bytes_received) == -1) {
            syslog(LOG_ERR, "Error writing to file");
            close(client_sockfd);
            new_thread_data->thread_node.thread_complete = 1;
            return new_thread_data;
        }
        for (int i = 0; i < sizeof(buffer); i++) {
            // Detect is it a packet
            if (buffer[i] == '\n') {
                // If it is a packet, seek to the beginning of the file and send the entire file content
                if (lseek(fd, 0, SEEK_SET) == -1) {
                    syslog(LOG_ERR, "Error seeking to beginning of file");
                    close(client_sockfd);
                    new_thread_data->thread_node.thread_complete = 1;
                    lock_flag = 0;
                    pthread_mutex_unlock(&mutex);
                    return new_thread_data;
                }
                while (1) {
                    ssize_t bytes_read = read(fd, file_buffer, sizeof(file_buffer));
                    if (bytes_read == -1) {
                        syslog(LOG_ERR, "Error reading from file");
                        close(client_sockfd);
                        new_thread_data->thread_node.thread_complete = 1;
                        lock_flag = 0;
                        pthread_mutex_unlock(&mutex);
                        return new_thread_data;
                    }
                    if (bytes_read == 0) {
                        lock_flag = 0;
                        pthread_mutex_unlock(&mutex);
                        break;
                    }
                    ssize_t bytes_sent = send(client_sockfd, file_buffer, bytes_read, 0);
                    if (bytes_sent == -1) {
                        syslog(LOG_ERR, "Error sending data to client");
                        close(client_sockfd);
                        new_thread_data->thread_node.thread_complete = 1;
                        lock_flag = 0;
                        pthread_mutex_unlock(&mutex);
                        return new_thread_data;
                    }
                }
                break;
            }
        }
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {

    fd = sockfd = -1;
    signal_flag = -1;
    servinfo = NULL;

    openlog("aesdsocket", LOG_PID, LOG_USER);

    TAILQ_HEAD(head_s, node) head;
    TAILQ_INIT(&head);

    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        syslog(LOG_ERR, "Error setting up signal handler");
        close_all();
        exit(-1);
    }

    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        syslog(LOG_ERR, "Error setting up signal handler");
        close_all();
        exit(-1);
    }

    // Open the file for writing
    fd = open(
        "/var/tmp/aesdsocketdata",
        O_RDWR | O_CREAT | O_APPEND,
        S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH
    );
    if (fd == -1) {
        syslog(LOG_ERR, "Error opening file");
        close_all();
        exit(-1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        syslog(LOG_ERR, "ERROR opening socket");
        close_all();
        exit(-1);
    }

    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, "9000", &hints, &servinfo) < 0) {
        syslog(LOG_ERR, "ERROR on getaddrinfo");
        close_all();
        exit(-1);
    }

    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
        syslog(LOG_ERR, "ERROR on binding");
        close_all();
        exit(-1);
    }

    if (argc == 2) {
        if (strcmp(argv[1], "-d") == 0) {
            pid_t pid;
            pid = fork();
            if (pid == -1) {
                syslog(LOG_ERR, "Error on fork");
                exit(-1);
            }
            else if (pid != 0) {
                exit(0);
            }
            if (setsid() == -1) {
                syslog(LOG_ERR, "Error on setsid");
                exit(-1);
            }
            if (chdir("/") == -1) {
                syslog(LOG_ERR, "Error on chdir");
                exit(-1);
            }
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
        }
    }

    pthread_t timer_thread_id;
    if (pthread_create(&timer_thread_id, NULL, timer_routine, NULL)) {
        syslog(LOG_ERR, "Error creating timer thread");
        close_all();
        exit(-1);
    }

    if (listen(sockfd, 10) < 0) {
        syslog(LOG_ERR, "ERROR on listen");
        close_all();
        exit(-1);
    }

    while (1) {

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0; // Non-blocking select
        int ret = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (ret == 0) {
            // No incoming connection, do something else here
            struct node *curr_node, *temp_node;
            struct thread_data *curr_thread_data;
            TAILQ_FOREACH_SAFE(curr_node, &head, entries, temp_node) {
                // If the thread is completed, join it
                if (curr_node->thread_complete) {
                    pthread_join(curr_node->thread_id, (void **)&curr_thread_data);
                    TAILQ_REMOVE(&head, curr_node, entries);
                    free(curr_thread_data);
                    break; // Restart iteration as list has changed
                }
            }
        } else if (ret > 0){
            int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
            if (client_sockfd < 0) {
                syslog(LOG_ERR, "ERROR on accept");
                close_all();
                exit(-1);
            }
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            syslog(LOG_INFO, "Accepted connection from %s", client_ip);

            // Create a new thread for each client connection
            struct thread_data *new_thread_data = malloc(sizeof(struct thread_data));
            if (new_thread_data == NULL) {
                syslog(LOG_ERR, "Error allocating memory for thread data");
                close_all();
                exit(-1);
            }

            TAILQ_INSERT_TAIL(&head, &(new_thread_data->thread_node), entries);

            new_thread_data->client_sockfd = client_sockfd;
            new_thread_data->thread_node.thread_complete = 0;
            strcpy(new_thread_data->client_ip, client_ip);

            if (pthread_create(&(new_thread_data->thread_node.thread_id), NULL, routine, new_thread_data)) {
                syslog(LOG_ERR, "Error creating thread");
                close_all();
                close(client_sockfd);
                exit(-1);
            }
        }

        if (signal_flag == SIGINT || signal_flag == SIGTERM) {
            syslog(LOG_INFO, "Caught signal, exiting");
            struct node *curr_node, *temp;
            struct thread_data *curr_thread_data;
            TAILQ_FOREACH_SAFE(curr_node, &head, entries, temp) {
                pthread_join(curr_node->thread_id, (void **)&curr_thread_data);
                free(curr_thread_data);
            }

            pthread_cancel(timer_thread_id);
            pthread_join(timer_thread_id, NULL);

            close_all();
            remove("/var/tmp/aesdsocketdata");
            exit(0);
        }
        
        
    }
    close_all();
    
    return 0;
}