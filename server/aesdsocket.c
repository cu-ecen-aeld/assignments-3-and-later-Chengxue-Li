#include "aesdsocket.h"
#include <limits.h>

static void signal_handler(int signo) {
    signal_flag = signo;
}

void close_all() {
    if (fd != -1) {
        close(fd);
    }
    if (client_sockfd != -1) {
        close(client_sockfd);
    }
    if (sockfd != -1) {
        close(sockfd);
    }
    if (servinfo != NULL) {
        freeaddrinfo(servinfo);
    }
    closelog();
}

int main(int argc, char *argv[]) {

    if (argc == 2) {
        if (strcmp(argv[1], "-d") == 0) {
            pid_t pid;
            int i;
            pid = fork();
            if (pid == -1) {
                exit(-1);
            }
            else if (pid != 0) {
                exit(0);
            }
            if (setsid() == -1) {
                exit(-1);
            }
            if (chdir("/") == -1) {
                exit(-1);
            }
            int fdlimit = (int)sysconf(_SC_OPEN_MAX);
            for (i = 0; i < fdlimit; i++) {
                close(i);
            }
            open("/dev/null", O_RDWR);
            dup(0);
            dup(0);
        }
    }

    fd = client_sockfd = sockfd = -1;
    signal_flag = -1;
    servinfo = NULL;

    openlog("aesdsocket", LOG_PID, LOG_USER);

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

        int ret = select(sockfd + 1, &readfds, NULL, NULL, NULL); // or &timeout for timeout
        if (ret < 0) {
            if (errno == EINTR) {
                if (signal_flag == SIGINT || signal_flag == SIGTERM) {
                    syslog(LOG_INFO, "Caught signal, exiting");
                    close_all();
                    remove("/var/tmp/aesdsocketdata");
                    exit(0);
                }
                continue; // Retry select
            }
            syslog(LOG_ERR, "Error on select");
            close_all();
            exit(-1);
        }
        
        client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (client_sockfd < 0) {
            syslog(LOG_ERR, "ERROR on accept");
            close_all();
            exit(-1);
        }
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        
        char buffer[10];
        char file_buffer[10];

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

        while (1) {
            memset(buffer, 0, sizeof(buffer));

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(client_sockfd, &readfds);

            int ret = select(client_sockfd + 1, &readfds, NULL, NULL, NULL); // or &timeout for timeout
            if (ret < 0) {
                if (errno == EINTR) {
                    if (signal_flag == SIGINT || signal_flag == SIGTERM) {
                        syslog(LOG_INFO, "Caught signal, exiting");
                        close_all();
                        remove("/var/tmp/aesdsocketdata");
                        exit(0);
                    }
                    continue; // Retry select
                }
                syslog(LOG_ERR, "Error on select");
                close_all();
                exit(-1);
            } else if (ret == 0) {
                continue;
            }

            // if (FD_ISSET(client_sockfd, &readfds)) {

            // }
            ssize_t bytes_received = recv(client_sockfd, buffer, sizeof(buffer), 0);
            if (bytes_received < 0) {
                syslog(LOG_ERR, "Error receiving data from client");
                close_all();
                exit(-1);
            }
            if (bytes_received == 0) {
                syslog(LOG_INFO, "Closed connection from %s\n", client_ip);
                close(fd);
                close(client_sockfd);
                break;
            }
            if (write(fd, buffer, bytes_received) == -1) {
                syslog(LOG_ERR, "Error writing to file");
                close_all();
                exit(-1);
            }
            for (int i = 0; i < sizeof(buffer); i++) {
                if (buffer[i] == '\n') {
                    if (lseek(fd, 0, SEEK_SET) == -1) {
                        syslog(LOG_ERR, "Error seeking to beginning of file");
                        close_all();
                        exit(-1);
                    }
                    while (1) {
                        ssize_t bytes_read = read(fd, file_buffer, sizeof(file_buffer));
                        if (bytes_read == -1) {
                            syslog(LOG_ERR, "Error reading from file");
                            close_all();
                            exit(-1);
                        }
                        if (bytes_read == 0) {
                            break;
                        }
                        ssize_t bytes_sent = send(client_sockfd, file_buffer, bytes_read, 0);
                        if (bytes_sent == -1) {
                            syslog(LOG_ERR, "Error sending data to client");
                            close_all();
                            exit(-1);
                        }
                    }
                    break;
                }
            }
        }
    }
    close_all();
    
    return 0;
}