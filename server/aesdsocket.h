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

int fd;
int client_sockfd;
int sockfd;
struct addrinfo *servinfo;

volatile int signal_flag;

static void signal_handler(int signo);
void close_all();
int main(int argc, char *argv[]);

#endif