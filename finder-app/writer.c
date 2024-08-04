#include <stdio.h>
#include <syslog.h>

int main(int argc, char **argv) {

    openlog(NULL, 0, LOG_USER);
    
    // check 2 args
    if (argc <= 2) {
        syslog(LOG_ERR, "No enough args");
        closelog();
        return 1;
    }

    // get filename and text from args
    char *filename = argv[1];
    char *text = argv[2];

    // write text to the file, exit with error code 1 if any error
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        syslog(LOG_ERR, "Cannot write text to file");
        closelog();
        return 1;
    }
    fprintf(file, "%s", text);
    fclose(file);
    syslog(LOG_DEBUG, "Writing %s to %s", text, filename);
    closelog();
    
    return 0;
}