#include <stdio.h>
#include <syslog.h>


int main(int argc, char *argv[]) {
    
    openlog(NULL,0,LOG_USER);
    if (argc != 2) {
        syslog(LOG_ERR,"Invalid Number of Arguments: %d",argc);
        return 1;
    }

    char *write_file = argv[0];
    char *write_string = argv[1];

    FILE *file = fopen(write_file, "w");
    if (file == NULL) {
        syslog(LOG_ERR, "Failed to open file for writing: %s", write_file);
        return 1;
    }

    syslog(LOG_DEBUG, "Writing %s to %s", write_string, write_file); 
    fprintf(file, "%s", write_string);
    fclose(file);

    syslog(LOG_INFO, "File Written Successfully");
    closelog();

    return 0;
}
