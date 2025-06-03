#ifndef AESDSOCKET_H
#define AESDSOCKET_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <linux/fs.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <time.h>
#include "queue.h"

struct node {
    pthread_t thread_id;
    int thread_complete; // Flag to indicate if the thread has completed
    TAILQ_ENTRY(node) entries;
};

struct thread_data {
    int client_sockfd;
    char client_ip[INET_ADDRSTRLEN];
    struct node thread_node;
};

int fd;
int sockfd;
struct addrinfo *servinfo;

volatile int signal_flag;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void signal_handler(int signo);
void close_all();
void *timer_routine(void *);
void *routine(void *);
int main(int argc, char *argv[]);

#endif