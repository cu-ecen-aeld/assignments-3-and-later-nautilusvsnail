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
#include <string.h>
#include <pthread.h>

#define PORT "9000"
#define BACKLOG 10
#define BUFFERSIZE 1024

// GLOBAL
int sock_fd = -1;
char *write_file = "/var/tmp/aesdsocketdata";

struct thread_data{
    pthread_t thread_id;
    int client_fd;
    FILE *file;
    struct sockaddr_storage client_addr;
    socklen_t client_len;
    pthread_mutex_t mutex;
    char client_ip_str[INET6_ADDRSTRLEN];
    char *write_file;
};

void shutdown_handler(void) {
    // Close the open file and sockets
    if (file != NULL) {
        syslog(LOG_INFO, "closing file");
        fclose(file);
    }
    if (client_fd >= 0) {
        syslog(LOG_INFO, "closing client_fd");
        close(client_fd);
    }
    if (sock_fd >= 0) {
        syslog(LOG_INFO, "closing sock_fd");
        close(sock_fd);
    }
    remove(write_file);
    closelog();
}


void handle_signal(int signal) {
    syslog(LOG_INFO, "Caught signal %d, shutting down gracefully...", signal);
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
        syslog(LOG_ERR, "Error setting SIGINT handler");
        return -1;
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Error setting SIGTERM handler");
        return -1;
    }
    return 0;
}


int initialize_socket() {
    syslog(LOG_INFO, "open for business");

    int ret;
    struct addrinfo hints, *res;

    // INITIALZE ADDRINFO HINTS
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    
    // GETADDRINFO
    ret = getaddrinfo(NULL, PORT, &hints, &res);
    if (ret != 0) {
        syslog(LOG_ERR, "getaddrinfo failure: %s", gai_strerror(ret));
        return -1;
    }

    // SOCKET
    int yes=1;
    sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd == -1) {
        syslog(LOG_ERR,"Failed to create socket");
        return -1;
    }
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != 0) {
        syslog(LOG_ERR, "Failed to set SO_REUSEADDR");
        return -1;
    }
    syslog(LOG_INFO, "socket initialized");

    // BIND
    ret = bind(sock_fd, res->ai_addr, res->ai_addrlen);
    if (ret == -1) {
        syslog(LOG_ERR, "bind failure: %s", strerror(errno));
        return -1;
    }
    freeaddrinfo(res);
    syslog(LOG_INFO, "bind complete");

    return 0;
}


int parse_client_ip(const struct sockaddr_storage *client_addr, char* client_ip_str, size_t client_ip_size) {

    if (client_addr->ss_family == AF_INET) { // IPv4
        struct sockaddr_in *s = (struct sockaddr_in *)client_addr;
        inet_ntop(AF_INET, &s->sin_addr, client_ip_str, client_ip_size);
    } else if (client_addr->ss_family == AF_INET6) { // IPv6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)client_addr;
        inet_ntop(AF_INET6, &s->sin6_addr, client_ip_str, client_ip_size);
    } else { return -1; }
    
    return 0;
}


int receive_data_to_file(struct thread_data *data) {
    char buffer[BUFFERSIZE];
    ssize_t bytes_received = 0;
    char *newline_ptr = NULL; // points to location in buffer
    memset(buffer, 0, BUFFERSIZE);
    
    // open file for writing
    data->file = fopen(write_file, "a");
    if (data->file == NULL) {
        syslog(LOG_ERR, "Failed to open file for writing: %s", write_file);
        return 1;
    }

    // Receive data and append packets to the file
    do {
        bytes_received = recv(data->client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received == -1) {
            syslog(LOG_ERR, "receive error: %s", strerror(errno));
            return -1;
        }

        newline_ptr = strchr(buffer, '\n');
        if (newline_ptr != NULL) {
            *(newline_ptr + 1) = '\0';
        }

        if (bytes_received > 0) {
            fprintf(data->file, "%s", buffer);
            fflush(data->file);
        }
        memset(buffer, 0, BUFFERSIZE);
    } while (newline_ptr == NULL && bytes_received > 0);

    fclose(data->file);
    data->file = NULL;

    return 0;
}


int send_data_from_file(struct thread_data *data) {
    char buffer[BUFFERSIZE];
    memset(buffer, 0, BUFFERSIZE);
    
    data->file = fopen(data->write_file, "r");  // Open the file for reading
    if (data->file == NULL) {
        syslog(LOG_ERR, "Failed to open file for sending");
        return -1;
    }

    // Buffer to hold the chunk of data
    size_t bytes_read = 0;

    // Read chunks of the file until the end is reached
    while ((bytes_read = fread(buffer, 1, sizeof(buffer) - 1, data->file)) > 0) {
        if (send(data->client_fd, buffer, bytes_read, 0) != bytes_read) {
            syslog(LOG_ERR, "error sending data");
            return -1;
        }
        memset(buffer, 0, BUFFERSIZE);
    }
    fclose(data->file);
    data->file = NULL;
    return 0;
}

void thread_read_write(void* thread_param) {
    struct thread_data *data = (struct thread_data *) thread_param;
    
    if (receive_data_to_file(data) != 0) {
        syslog(LOG_ERR, "receive failure");
        return -1;
    };

    if (send_data_from_file(data) != 0) {
        syslog(LOG_ERR, "send error");
        return -1;
    }

    close(data->client_fd);
    data->client_fd = -1;
    syslog(LOG_INFO, "Closed connection from %s", data->client_ip_str);
    free(data);
}


int main(int argc, char *argv[]) {
    int daemon_flag = -1; // false
    int option;
    pthread_mutex_t mutex;

    // Parse command-line options
    while ((option = getopt(argc, argv, "d")) != -1) {
        switch (option) {
            case 'd':
                daemon_flag = 0;  // true
                break;
            case '?':  // Invalid option
                syslog(LOG_ERR, "Unknown option: -%c", optopt);
                return -1;
            default:
                return -1;
        }
    }
    
    int ret;

    openlog(NULL,0,LOG_USER);

    if(atexit(shutdown_handler) != 0) {
        syslog(LOG_ERR, "failed to register exit handler");
        return -1;
    };

    // Signal handlers for SIGINT and SIGTERM
    if (setup_signal_handler() == -1) {
        syslog(LOG_ERR, "signal setup fail");
        return -1;
    }

    if (initialize_socket() == -1) {
        syslog(LOG_ERR, "signal setup fail");
        return -1;
    }

    if (daemon_flag == 0) {
        int kidpid = fork();
        if (kidpid < 0) {
            syslog(LOG_ERR, "failed to initialize daemon");
            return -1;
        } else if (kidpid == 0) {
            if (freopen("/dev/null", "r", stdin) == NULL) {
                syslog(LOG_ERR, "Failed to redirect stdin to /dev/null");
            }
            if (freopen("/dev/null", "w", stdout) == NULL) {
                syslog(LOG_ERR, "Failed to redirect stdout to /dev/null");
            }
            if (freopen("/dev/null", "w", stderr) == NULL) {
                syslog(LOG_ERR, "Failed to redirect stderr to /dev/null");
            }

            syslog(LOG_INFO, "daemon initialized successfully");
        } else {
            return 0;
        }
    }

    // LISTEN
    ret = listen(sock_fd, BACKLOG);
    if (ret == -1) {
        syslog(LOG_ERR, "listen failure: %s", strerror(errno));
        return -1;
    }
    syslog(LOG_INFO, "listening");


    while (1) {
        struct thread_data *data = (struct thread_data *)malloc(sizeof(struct thread_data));
        data->file = NULL;
        data->mutex = mutex;
        data->client_len = sizeof(data->client_addr);
        data->write_file = write_file;
        
        // ACCEPT
        ret = accept(sock_fd, (struct sockaddr*)&(data->client_addr), &(data->client_len));
        if (ret == -1) {
            syslog(LOG_ERR, "Accept failure: %s", strerror(errno));
            free(data);
            return -1;
        } else {
            data->client_fd = ret;
        }
        
        // Get the client's IP address
        if (parse_client_ip(&(data->client_addr), (char *)&(data->client_ip_str), sizeof(data->client_ip_str)) == -1) {
            syslog(LOG_ERR, "failed to parse client ip");
            free(data);
            return -1;
        }
        syslog(LOG_INFO, "Accepted connection from %s", data->client_ip_str);

        thread_read_write((void *)data);
        ret = pthread_create(data->thread_id, NULL, thread_read_write, (void *)data);
        if (ret == 0) {
            DEBUG_LOG("Thread Started: %ld", (long)(data->thread_id));
            return 0;
        } else {
            ERROR_LOG("Thread Start Failed with error %s", strerror(ret));
            free(data);
            return -1;
        }
    }
    return 0;
}