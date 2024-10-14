#define _XOPEN_SOURCE 600

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>


#define PORT "9000"
#define BACKLOG 10
#define BUFFERSIZE 1024

int sock_fd = -1;
int client_fd = -1;
FILE *file = NULL;


void handle_signal(int signal) {
    syslog(LOG_INFO, "AESDSOCKET: Caught signal %d, shutting down gracefully...", signal);
    
    // Close the open file and sockets
    if (file != NULL) {
        syslog(LOG_INFO, "AESDSOCKET: closing file");
        fclose(file);
    }
    if (client_fd >= 0) {
        syslog(LOG_INFO, "AESDSOCKET: closing client_fd");
        close(client_fd);
    }
    if (sock_fd >= 0) {
        syslog(LOG_INFO, "AESDSOCKET: closing sock_fd");
        close(sock_fd);
    }

    syslog(LOG_INFO, "AESDSOCKET: Caught signal, exiting");
    closelog();

    // Exit the program
    exit(0);
}

int setup_signal_handler() {
    struct sigaction sa;

    // Clear the sigaction struct
    memset(&sa, 0, sizeof(sa));

    // Set the signal handler function
    sa.sa_handler = handle_signal;

    // Ensure all signals are unblocked during the handler
    sigemptyset(&sa.sa_mask);

    // Set flags to ensure we use the standard behavior
    sa.sa_flags = 0;

    // Register the signal handler for SIGINT and SIGTERM
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        syslog(LOG_ERR, "AESDSOCKET: Error setting SIGINT handler");
        return -1;
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        syslog(LOG_ERR, "AESDSOCKET: Error setting SIGTERM handler");
        return -1;
    }
    return 0;
}


int main(int argc, char *argv[]) {

    openlog(NULL,0,LOG_USER);

    syslog(LOG_INFO, "AESDSOCKET: open for business");

    int ret;
    struct addrinfo hints, *res;
    struct sockaddr_storage client_addr;
    socklen_t client_len = sizeof(client_addr);
    char client_ip_str[INET6_ADDRSTRLEN];
    
    // Set up signal handlers for SIGINT and SIGTERM
    if (setup_signal_handler() == -1) {
        syslog(LOG_ERR, "AESDSOCKET: signal setup fail");
        return -1;
    }

    // INITIALZE ADDRINFO HINTS
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    
    // GETADDRINFO

    ret = getaddrinfo(NULL, PORT, &hints, &res);
    if (ret != 0) {
        syslog(LOG_ERR, "AESDSOCKET: getaddrinfo failure: %s", gai_strerror(ret));
        return -1;
    }


    // SOCKET

    sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd == -1) {
        syslog(LOG_ERR,"AESDSOCKET: Failed to create socket");
        return -1;
    }
    syslog(LOG_INFO, "AESDSOCKET: socket initialized");

    // BIND

    ret = bind(sock_fd, res->ai_addr, res->ai_addrlen);
    if (ret == -1) {
        syslog(LOG_ERR, "AESDSOCKET: bind failure: %s", strerror(errno));
        return -1;
    }
    freeaddrinfo(res);
    syslog(LOG_INFO, "AESDSOCKET: bind complete");

    // LISTEN

    ret = listen(sock_fd, BACKLOG);
    if (ret == -1) {
        syslog(LOG_ERR, "AESDSOCKET: listen failure: %s", strerror(errno));
        return -1;
    }
    syslog(LOG_INFO, "AESDSOCKET: listening");

    // ACCEPT
    // todo: make a loop
    while (1) {
        client_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_len);
        if (ret == -1) {
            syslog(LOG_ERR, "AESDSOCKET: Accept failure: %s", strerror(errno));
            return -1;
        }
        syslog(LOG_INFO, "AESDSOCKET: accepted connection");
        
        // Get the client's IP address
        // todo: functionalize this

        if (client_addr.ss_family == AF_INET) { // IPv4
            struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
            inet_ntop(AF_INET, &s->sin_addr, client_ip_str, sizeof client_ip_str);
        } else if (client_addr.ss_family == AF_INET6) { // IPv6
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_addr;
            inet_ntop(AF_INET6, &s->sin6_addr, client_ip_str, sizeof client_ip_str);
        }

        syslog(LOG_INFO, "AESDSOCKET: Accepted connection from %s", client_ip_str);


        // open file for writing
        char *write_file = "/var/tmp/aesdsocketdata";
        file = fopen(write_file, "a");
        if (file == NULL) {
            syslog(LOG_ERR, "AESDSOCKET: Failed to open file for writing: %s", write_file);
            return 1;
        }

        // RECV
        // Read data from the socket
        
        char buffer[BUFFERSIZE];
        ssize_t bytes_received;
        char *newline_ptr;
        size_t buffer_offset = 0;

        // Receive data and append packets to the file
        while ((bytes_received = recv(client_fd, buffer + buffer_offset, sizeof(buffer) - 1 - buffer_offset, 0)) > 0) {
            bytes_received += buffer_offset;  // Include leftover data
            buffer[bytes_received] = '\0';    // Null-terminate the received data

            newline_ptr = strchr(buffer, '\n');
            while (newline_ptr != NULL) {
                size_t packet_size = newline_ptr - buffer + 1;
                
                // Temporarily null-terminate the packet
                char saved_char = *(newline_ptr + 1);
                *(newline_ptr + 1) = '\0'; 

                // Write the packet to the file
                fprintf(file, "%s", buffer);
                fflush(file);

                // Restore the buffer
                *(newline_ptr + 1) = saved_char;

                // Move any remaining data after the newline to the start of the buffer
                size_t remaining_data_size = bytes_received - packet_size;
                if (remaining_data_size > 0) {
                    memmove(buffer, newline_ptr + 1, remaining_data_size);
                    bytes_received = remaining_data_size;
                } else {
                    bytes_received = 0;
                }

                // Look for another newline in the remaining data
                newline_ptr = strchr(buffer, '\n');
            }

            buffer_offset = bytes_received;  // Store leftover data for the next iteration
        }
        close(client_fd);
        client_fd = -1;
        fclose(file);
        file = NULL;
    }
    syslog(LOG_INFO, "AESDSOCKET: shutdown");
    closelog();
    return 0;
}
