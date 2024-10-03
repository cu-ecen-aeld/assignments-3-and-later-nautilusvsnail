// usage: writer <file> <string>
// creates file at <file> if it does not exist
//   - <file> must be fully qualified path
// overwrites any existing data in <file> with <string>


#include <stdio.h>
#include <syslog.h>


int main(int argc, char *argv[]) {
    
    openlog(NULL,0,LOG_USER);
    if (argc != 3) {
        syslog(LOG_ERR,"Invalid Number of Arguments: %d",argc - 1);
        return 1;
    }

    char *write_file = argv[1];
    char *write_string = argv[2];

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
