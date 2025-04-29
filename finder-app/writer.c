#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {    
    int fd;

    // Open syslog
    openlog("writer", LOG_PID, LOG_USER);

    // if args is less than 2, return 1
    if (argc <= 2) {
        syslog(LOG_ERR, "Args are less than 2");
        return 1;
    }

    // Open the file for writing
    fd = open(
        argv[1],
        O_WRONLY | O_CREAT | O_TRUNC,
        S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH
    );
    if (fd == -1) {
        syslog(LOG_ERR, "Error opening file");
        return 1;
    }

    // Write data to the file
    if (write(fd, argv[2], strlen(argv[2])) == -1) {
        syslog(LOG_ERR, "Error writing to file");
        close(fd);
        return 1;
    }

    // Log the message
    syslog(LOG_DEBUG, "Writing '%s' to '%s'.", argv[2], argv[1]);

    // Close the file
    if (close(fd) == -1) {
        syslog(LOG_ERR, "Error closing file");
        return 1;
    }

    // Close syslog
    closelog();

    return 0;
}