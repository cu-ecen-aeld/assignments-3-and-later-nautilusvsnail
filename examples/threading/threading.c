#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    struct thread_data *thread_func_args = (struct thread_data *) thread_param;
    int rc;

    DEBUG_LOG("Thread %ld Initialized", (long)*(thread_func_args->thread_id));
    usleep(thread_func_args->wait_to_obtain_ms * 1000);

    DEBUG_LOG("Attempting to obtain mutex for thread %ld", (long)*(thread_func_args->thread_id));
    rc = pthread_mutex_lock(thread_func_args->mutex);
    if (rc != 0) {
        thread_func_args->thread_complete_success = false;
        ERROR_LOG("Thread %ld mutex log failed with error %s", (long)*(thread_func_args->thread_id), strerror(rc));
    } else {
        DEBUG_LOG("Thread %ld successfully obtained mutex", (long)*(thread_func_args->thread_id));
    }
    usleep(thread_func_args->wait_to_release_ms * 1000);
    
    rc = pthread_mutex_unlock(thread_func_args->mutex);
    if (rc != 0) {
        thread_func_args->thread_complete_success = false;
        ERROR_LOG("Thread %ld mutex release failed with error %s", (long)*(thread_func_args->thread_id), strerror(rc));
    } else {
        DEBUG_LOG("Thread %ld successfully released mutex", (long)*(thread_func_args->thread_id));
    }

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    DEBUG_LOG("beginning start_thread_obtaining_mutex");
    int rc;

    // Ensure the mutex pointer is valid
    if (mutex == NULL) {
        ERROR_LOG("Invalid mutex pointer");
        return false;
    }

    struct thread_data *data = (struct thread_data *)malloc(sizeof(struct thread_data));
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->mutex = mutex;
    data->thread_complete_success = true;
    data->thread_id = thread;
    DEBUG_LOG("struct successfully populated");
    
    rc = pthread_create(data->thread_id, NULL, threadfunc, (void *)data);
    if (rc == 0) {
        DEBUG_LOG("Thread Started: %ld", (long)*(data->thread_id));
        return true;
    } else {
        ERROR_LOG("Thread Start Failed with error %s", strerror(rc));
        free(data->thread_id);
        free(data);
        return false;
    }
}