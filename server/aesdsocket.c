#define _XOPEN_SOURCE 600
#define __USE_MISC

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
#include <sys/queue.h>
#include <stdbool.h>

// TYPES
struct thread_data
{
    pthread_t thread_id;
    int client_fd;
    FILE *file;
    struct sockaddr_storage client_addr;
    socklen_t client_len;
    pthread_mutex_t *mutex;
    char client_ip_str[INET6_ADDRSTRLEN];
    bool thread_complete;
};

struct threadlist_entry
{
    struct thread_data *data;
    STAILQ_ENTRY(threadlist_entry) next_entry; // list pointer
};

STAILQ_HEAD(threadlist_head_type, threadlist_entry);

// GLOBAL CONSTANTS
const char *write_file = "/var/tmp/aesdsocketdata";
const char *PORT = "9000";
const int BACKLOG = 10;
const int BUFFERSIZE = 1024;
const int SUCCESS = 0;
pthread_mutex_t mutex;

// GLOBAL VARIABLES
int sock_fd = -1;
volatile sig_atomic_t terminate = false;
struct threadlist_head_type threadlist_head;

// SUPPORT FUNCTIONS
void thread_cleanup() 
{
    struct threadlist_entry *current_entry = STAILQ_FIRST(&threadlist_head);
    struct threadlist_entry *tmp_next;

    while(current_entry != NULL) {
        syslog(LOG_INFO, "in the out door");
        tmp_next = STAILQ_NEXT(current_entry, next_entry);
        struct thread_data *data = current_entry->data;
        
        if (data->thread_id > 0) {
            pthread_join(data->thread_id, NULL); // does not currently check thread exit status
        }

        if(data->client_fd >= 0) {
            close(data->client_fd);
            data->client_fd = -1;
            syslog(LOG_INFO, "Closed connection from %s", data->client_ip_str);
        }
        
        if (data->file != NULL) {
            fclose(data->file);
            data->file = NULL;
        }

        STAILQ_REMOVE(&threadlist_head, current_entry, threadlist_entry, next_entry);
        free(data);
        free(current_entry);
        current_entry = tmp_next;
    }
}

// void exit_handler(void) 
// {
//     // struct threadlist_head_type *threadlist_head = (struct threadlist_head_type *)arg;
//     if (sock_fd >= 0)
//     {
//         syslog(LOG_INFO, "closing sock_fd");
//         close(sock_fd);
//     }
//     remove(write_file);
//     closelog();
//     thread_cleanup();
// }

void handle_signal(int signal)
{
    syslog(LOG_INFO, "Caught signal %d, shutting down gracefully...", signal);
    
    thread_cleanup(&threadlist_head);
    pthread_mutex_destroy(&mutex);

    terminate = true;
}

int setup_signal_handler()
{
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
    if (sigaction(SIGINT, &sa, NULL) != SUCCESS)
    {
        syslog(LOG_ERR, "Error setting SIGINT handler");
        return -1;
    }

    if (sigaction(SIGTERM, &sa, NULL) != SUCCESS)
    {
        syslog(LOG_ERR, "Error setting SIGTERM handler");
        return -1;
    }
    return 0;
}

int initialize_socket()
{
    syslog(LOG_INFO, "open for business");

    struct addrinfo hints, *res;

    // INITIALZE ADDRINFO HINTS
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    // GETADDRINFO
    int ret = getaddrinfo(NULL, PORT, &hints, &res);
    if (ret != 0)
    {
        syslog(LOG_ERR, "getaddrinfo failure: %s", gai_strerror(ret));
        return -1;
    }

    // SOCKET
    int yes = 1;
    sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd == -1)
    {
        syslog(LOG_ERR, "Failed to create socket");
        return -1;
    }
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != 0)
    {
        syslog(LOG_ERR, "Failed to set SO_REUSEADDR");
        return -1;
    }
    syslog(LOG_INFO, "socket initialized");

    // BIND
    if (bind(sock_fd, res->ai_addr, res->ai_addrlen) != SUCCESS)
    {
        syslog(LOG_ERR, "bind failure: %s", strerror(errno));
        return -1;
    }
    freeaddrinfo(res);
    syslog(LOG_INFO, "bind complete");

    return 0;
}

int parse_client_ip(const struct sockaddr_storage *client_addr, char *client_ip_str, size_t client_ip_size)
{

    if (client_addr->ss_family == AF_INET)
    { // IPv4
        struct sockaddr_in *s = (struct sockaddr_in *)client_addr;
        inet_ntop(AF_INET, &s->sin_addr, client_ip_str, client_ip_size);
    }
    else if (client_addr->ss_family == AF_INET6)
    { // IPv6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)client_addr;
        inet_ntop(AF_INET6, &s->sin6_addr, client_ip_str, client_ip_size);
    }
    else
    {
        return -1;
    }

    return 0;
}

// THREAD FUNCTIONS
int receive_data_to_file(struct thread_data *data)
{
    char buffer[BUFFERSIZE];
    ssize_t bytes_received = 0;
    char *newline_ptr = NULL; // points to location in buffer
    memset(buffer, 0, BUFFERSIZE);

    // open file for writing
    data->file = fopen(write_file, "a");
    if (data->file == NULL)
    {
        syslog(LOG_ERR, "Failed to open file for writing: %s", write_file);
        return 1;
    }

    // Receive data and append packets to the file
    do {
        bytes_received = recv(data->client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received == -1)
        {
            syslog(LOG_ERR, "receive error: %s", strerror(errno));
            fclose(data->file);
            data->file = NULL;
            return -1;
        }

        newline_ptr = strchr(buffer, '\n');
        if (newline_ptr != NULL)
        {
            *(newline_ptr + 1) = '\0';
        }

        if (bytes_received > 0)
        {
            fprintf(data->file, "%s", buffer);
            fflush(data->file);
        }
        memset(buffer, 0, BUFFERSIZE);
    } while (newline_ptr == NULL && bytes_received > 0);

    fclose(data->file);
    data->file = NULL;

    return 0;
}

int send_data_from_file(struct thread_data *data)
{
    char buffer[BUFFERSIZE];
    memset(buffer, 0, BUFFERSIZE);

    data->file = fopen(write_file, "r"); // Open the file for reading
    if (data->file == NULL)
    {
        syslog(LOG_ERR, "Failed to open file for sending");
        return -1;
    }

    // Buffer to hold the chunk of data
    size_t bytes_read = 0;

    // Read chunks of the file until the end is reached
    while ((bytes_read = fread(buffer, 1, sizeof(buffer) - 1, data->file)) > 0)
    {
        if (send(data->client_fd, buffer, bytes_read, 0) != bytes_read)
        {
            syslog(LOG_ERR, "error sending data");
            fclose(data->file);
            data->file = NULL;
            return -1;
        }
        memset(buffer, 0, BUFFERSIZE);
    }
    fclose(data->file);
    data->file = NULL;
    return 0;
}

void *thread_read_write(void *thread_param)
{
    struct thread_data *data = (struct thread_data *)thread_param;
    int ret;

    ret = pthread_mutex_lock(data->mutex);
    if (ret != 0) {
        syslog(LOG_ERR, "Thread %ld mutex lock failed with error %s", (long)data->thread_id, strerror(ret));
        return NULL;
    }

    if (receive_data_to_file(data) != 0)
    {
        syslog(LOG_ERR, "receive failure");
        data->thread_complete = true;
        return NULL;
    };

    if (send_data_from_file(data) != 0)
    {
        syslog(LOG_ERR, "send error");
        data->thread_complete = true;
        return NULL;
    }

    ret = pthread_mutex_unlock(data->mutex);
    if (ret != 0) {
        syslog(LOG_ERR, "Thread %ld mutex unlock failed with error %s", (long)data->thread_id, strerror(ret));
        return NULL;
    }

    data->thread_complete = true;
    return NULL;
}

// MAIN FUNCTION
int main(int argc, char *argv[])
{
    bool daemon_flag = false;
    
    STAILQ_INIT(&threadlist_head);
    pthread_mutex_init(&mutex, NULL);

    // Command-line Arguments
    int option;
    while ((option = getopt(argc, argv, "d")) != -1)
    {
        switch (option)
        {
        case 'd':
            daemon_flag = true;
            break;
        case '?': // Invalid option
            syslog(LOG_ERR, "Unknown option: -%c", optopt);
            return -1;
        default:
            return -1;
        }
    }

    // Log
    openlog(NULL, 0, LOG_USER);

    // Exit Handler
    // if (atexit(exit_handler) != SUCCESS)
    // {
    //     syslog(LOG_ERR, "failed to register exit handler");
    //     return -1;
    // };

    // Signal handlers for SIGINT and SIGTERM
    if (setup_signal_handler() != SUCCESS)
    {
        syslog(LOG_ERR, "signal setup fail");
        return -1;
    }

    if (initialize_socket() != SUCCESS)
    {
        syslog(LOG_ERR, "signal setup fail");
        return -1;
    }

    // Daemon Mode
    if (daemon_flag)
    {
        int kidpid = fork();
        if (kidpid < 0)
        {
            syslog(LOG_ERR, "failed to initialize daemon");
            return -1;
        }
        else if (kidpid == 0)
        {
            if (freopen("/dev/null", "r", stdin) == NULL)
            {
                syslog(LOG_ERR, "Failed to redirect stdin to /dev/null");
            }
            if (freopen("/dev/null", "w", stdout) == NULL)
            {
                syslog(LOG_ERR, "Failed to redirect stdout to /dev/null");
            }
            if (freopen("/dev/null", "w", stderr) == NULL)
            {
                syslog(LOG_ERR, "Failed to redirect stderr to /dev/null");
            }

            syslog(LOG_INFO, "daemon initialized successfully");
        }
        else
        {
            return 0;
        }
    }

    if (listen(sock_fd, BACKLOG) != SUCCESS)
    {
        syslog(LOG_ERR, "listen failure: %s", strerror(errno));
        return -1;
    }

    syslog(LOG_INFO, "listening");

    // Main Program Loop
    while (!terminate)
    {
        // instantiate new thread_data
        struct thread_data *data = (struct thread_data*)malloc(sizeof(struct thread_data));
        data->thread_id = -1;
        data->client_fd = -1;
        data->file = NULL;
        memset(&(data->client_addr), 0, sizeof(data->client_addr));
        data->client_len = sizeof(data->client_addr);
        data->mutex = &mutex;
        memset(data->client_ip_str, 0, sizeof(data->client_ip_str));
        data->thread_complete = false;
        

        // add thread data to list
        struct threadlist_entry *entry = (struct threadlist_entry*)malloc(sizeof(struct threadlist_entry));
        if (entry == NULL) {
            syslog(LOG_ERR, "Failed to allocate memory for threadlist_entry");
            free(data);
            return -1;
        }
        
        entry->data = data;
        STAILQ_INSERT_TAIL(&threadlist_head, entry, next_entry);

        // ACCEPT
        data->client_fd = accept(sock_fd, (struct sockaddr *)&(data->client_addr), &(data->client_len)); 
        if (data->client_fd == -1)
        {
            syslog(LOG_ERR, "Accept failure: %s", strerror(errno));
            return -1;
        }

        // Get the client's IP address
        if (parse_client_ip(&(data->client_addr), (char *)&(data->client_ip_str), sizeof(data->client_ip_str)) != SUCCESS)
        {
            syslog(LOG_ERR, "failed to parse client ip");
            return -1;
        }

        syslog(LOG_INFO, "Accepted connection from %s", data->client_ip_str);

        int ret = pthread_create(&(data->thread_id), NULL, thread_read_write, (void *)data);
        if (ret != SUCCESS) {
            syslog(LOG_ERR, "Thread Start Failed with Error %s", strerror(ret));
            return -1;
        }

        struct threadlist_entry *this_entry = STAILQ_FIRST(&threadlist_head);
        struct threadlist_entry *tmp_next_entry;
        while (this_entry != NULL)
        {
            tmp_next_entry = STAILQ_NEXT(this_entry, next_entry);
            if (this_entry->data->thread_complete)
            {
                struct thread_data* this_data = this_entry->data;
                pthread_join(this_data->thread_id, NULL); // does not currently check thread exit status
                close(this_data->client_fd);
                this_data->client_fd = -1;
                syslog(LOG_INFO, "Closed connection from %s", this_data->client_ip_str);
                if (this_data->file != NULL) {
                    fclose(this_data->file);
                    this_data->file = NULL;
                }
                STAILQ_REMOVE(&threadlist_head, this_entry, threadlist_entry, next_entry);
                free(this_data);
            }
            this_entry = tmp_next_entry;
        }
    }
    return 0;
}