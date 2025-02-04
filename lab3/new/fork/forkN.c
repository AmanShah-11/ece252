/* The code is 
 * Copyright(c) 2018-2019 Yiqing Huang, <yqhuang@uwaterloo.ca>.
 *
 * This software may be freely redistributed under the terms of the X11 License.
 */

/**
 * @file   forkN.c
 * @brief  fork N child processes and time the overall execution time  
 */

/******************************************************************************
 * INCLUDE HEADER FILES
 *****************************************************************************/

#include <curl/curl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <semaphore.h>
#include "shm_stack.h"
#include <stdbool.h>


#define SHM_SIZE 256
#define NUM_CHILD 6
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 1024000
#define STACK_SIZE 50
void push_all(struct int_stack *p, int start);
void pop_all(struct int_stack *p);
void test_local();
void test_shm();


typedef struct {
    struct int_stack pic_stack;
    struct int_stack pic_int_stack;
    int p_num_strip;
    int c_num_strip;
    sem_t spaces;
    sem_t items;
    pthread_mutex_t p_mutex;
    pthread_mutex_t c_mutex;
    pthread_mutex_t stack_mutex;
} shared_data;

typedef struct recv_buf_flat {
    char *buf;       /* memory to hold a copy of received data */
    size_t size;     /* size of valid data in buf in bytes*/
    size_t max_size; /* max capacity of buf in bytes*/
    int seq;         /* >=0 sequence number extracted from http header */
    /* <0 indicates an invalid seq number */
} RECV_BUF;

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    RECV_BUF *p = userdata;
    if (realsize > strlen(ECE252_HEADER) &&
        strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {

        /* extract img sequence number */
        p->seq = atoi(p_recv + strlen(ECE252_HEADER));

    }
//    printf("Header CB is being called3\n");
    return realsize;
}

size_t write_cb_curl(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
//    printf("Write CB is being called\n");
    size_t realsize = size * nmemb;
    RECV_BUF *p = (RECV_BUF *)p_userdata;

    if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */
        fprintf(stderr, "User buffer is too small, abort...\n");
        abort();
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}

int shm_recv_buf_init(RECV_BUF *ptr, size_t nbytes)
{
    if ( ptr == NULL ) {
        return 1;
    }

    ptr->buf = (char *)ptr + sizeof(RECV_BUF);
    ptr->size = 0;
    ptr->max_size = nbytes;
    ptr->seq = -1;              /* valid seq should be non-negative */

    return 0;
}

int sizeof_shm_recv_buf(size_t nbytes)
{
    return (sizeof(RECV_BUF) + sizeof(char) * nbytes);
}

int write_file(const char *path, const void *in, size_t len)
{
    FILE *fp = NULL;

    if (path == NULL) {
        fprintf(stderr, "write_file: file name is null!\n");
        return -1;
    }

    if (in == NULL) {
        fprintf(stderr, "write_file: input data is null!\n");
        return -1;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror("fopen");
        return -2;
    }

    if (fwrite(in, 1, len, fp) != len) {
        fprintf(stderr, "write_file: imcomplete write!\n");
        return -3;
    }
    return fclose(fp);
}

int consume(int n, int shmid, int sleepTime, shared_data * shared_data_temp);
int produce(int n, int shmid, shared_data * shared_data_temp);

int consume(int n, int shmid, int sleepTime, shared_data * shared_data_temp)
{
//    struct int_stack *pstack;
//    pstack = shmat(shmid, NULL, 0);
//    if ( pstack == (void ) -1 ) {
//        perror("shmat");
//        abort();
//    }
//    int temp = n;
//    push(pstack, temp);
//    printf("child: pstack = %p\n", pstack);
//    usleep(1000sleepTime);
//    printf("Worker ID=%d, pid = %d, ppid = %d.\n", n, getpid(), getppid());
//
//    if ( shmdt(pstack) != 0 ) {
//        perror("shmdt");
//        abort();
//    }
//
//    return 0;

//    int size = 0;
//    int seq = 0;
//
//    // char fname[256];
//    RECV_BUF buf;
//    usleep(1000sleepTime);
//    printf("starting consumer\n");
//
//    /* if stack not empty */
//    sem_wait(&shared_data_temp->items);
//    if (is_empty(pic_stack) == 0)
//    {
//
//        int seq = pic_stack->items[pic_stack->pos].seq;
//        int size = pic_stack->items[pic_stack->pos].size;
//        char *buffer = malloc(size * sizeof(char));
//
//
//        pthread_mutex_lock(&shared_data_temp->stack_mutex);
//        // int file_num;
//        int *var = (int *) buffer; //.buf
//        pop(pic_stack, var);
//        char * newBuffer =  (char*) var;
//        // pop(pic_int_stack, &file_num);
//
//        char formatstring[256];
//        file_names[shared_data_temp->p_numstrip] = malloc(sizeof(formatstring));
//        snprintf(formatstring, 256, "output%i_%jd.png", shared_data_temp->p_num_strip, pid);
//        strcpy(file_names[shared_data_temp->p_num_strip], formatstring);
//
//        pthread_mutex_unlock(&shared_data_temp->stackmutex);
//
//        sprintf(fname, "./output%d_%d.png", seq, pid);
//        write_file(fname, newBuffer, size);
//
//        free(buffer);
//
//    }
//    sem_post(&shared_data_temp->spaces);
//
//    printf("I am the worker number %i with process %i\n", n, shmid);
//    return 0;

    while(true){
        int consume_count;
        pthread_mutex_lock(&shared_data_temp->c_mutex);
        shared_data_temp->c_num_strip += 1;
        consume_count = shared_data_temp->c_num_strip;
        pthread_mutex_unlock(&shared_data_temp->c_mutex);

        if (consume_count <= 50){
            int *temp_data;
            int *img_num;

            sem_wait(&shared_data_temp->items);
            pthread_mutex_lock(&shared_data_temp->stack_mutex);
            pop(&shared_data_temp->pic_stack, temp_data);
            pop(&shared_data_temp->pic_int_stack, img_num);
            pthread_mutex_unlock(&shared_data_temp->stack_mutex);
            sem_post(&shared_data_temp->spaces);

            char * newBuffer = (char*) temp_data;
            char formatstring[256];
            long int pid = 999999;
            snprintf(formatstring, 256, "output%i_%jd.png", *img_num, pid);
            write_file(formatstring, newBuffer, 20000);
        } else{
            break;
        }
    }
    return 0;
}

int produce(int n, int shmid, shared_data * shared_data_temp){
    /* Get shared data */
//    shared_data *shared_data_temp;
//    shared_data_temp = shmat(shmid, NULL, 0);
////    shared_data *shared_data_temp = shmat(shmid, NULL, 0);
//    if ( shared_data_temp == (void *) -1 ) {
//        perror("shmat");
//        abort();
//    }

    int value;
    sem_getvalue(&shared_data_temp->spaces, &value);
    printf("The value of my semaphore is %i (in produce) fork %i\n", value, n);


    /* Infinite while loop until we finish all 50 strips*/
    while(true){
        printf("In while loop\n");
        int produce_count;
        /* Critical section for obtaining strip num */
        pthread_mutex_lock(&shared_data_temp->p_mutex);
        shared_data_temp->p_num_strip += 1;
        produce_count = shared_data_temp->p_num_strip;
        pthread_mutex_unlock(&shared_data_temp->p_mutex);
        printf("Produce count is %d %d\n", produce_count, n);

        if (produce_count <= 50){
            printf("Being called on the %d th time", produce_count);
            /* Make cURL call to server */
            /* specify URL to get */
            char img_buffer[100];
            sprintf(img_buffer, "http://ece252-1.uwaterloo.ca:2530/image?img=1&part=%d", produce_count);
            printf("%s", img_buffer);
//            char img_buffer[100] = "http://ece252-1.uwaterloo.ca:2530/image?img=1&part=20";

            printf("img buffer\n");

            CURL *curl_handle;
            CURLcode res;
            RECV_BUF *p_shm_recv_buf;
            int shmid_buff;
            int shm_size = sizeof_shm_recv_buf(BUF_SIZE);

            printf("shm_size = %d.\n", shm_size);
            shmid_buff = shmget(IPC_PRIVATE, shm_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
            if ( shmid == -1 ) {
                perror("shmget");
                abort();
            }

            p_shm_recv_buf = shmat(shmid_buff, NULL, 0);
            shm_recv_buf_init(p_shm_recv_buf, BUF_SIZE);

//
            curl_global_init(CURL_GLOBAL_DEFAULT);
//
//            /* init a curl session */
            curl_handle = curl_easy_init();

            if (curl_handle == NULL) {
                fprintf(stderr, "curl_easy_init: returned NULL\n");
                return 1;
            }

//            printf("after easy init\n");

            /* register write call back function to process received data */
            curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl);

//            printf("After curl write function\n");
            curl_easy_setopt(curl_handle, CURLOPT_URL, img_buffer);

//            printf("write cb curl\n");

            /* user defined data structure passed to the call back function */
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)p_shm_recv_buf);

//            printf("write opt write data\n");

            /* register header call back function to process received header data */
            curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl);

//            printf("write opt header function \n");

            /* user defined data structure passed to the call back function */
            curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)p_shm_recv_buf);

//            printf("write opt header data \n");

            /* some servers requires a user-agent field */
            curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

//            printf("write lib curl agent \n");

            /* get it! */
            res = curl_easy_perform(curl_handle);
//            printf("Performing curl easy perform\n");

            if( res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            } else {
                printf("%lu bytes received in memory %p, seq=%d.\n",  \
                   p_shm_recv_buf->size, p_shm_recv_buf->buf, p_shm_recv_buf->seq);
                printf("The strip num %i and data%p \n", produce_count, p_shm_recv_buf->buf);

            }
//            printf("Done Performing curl easy perform\n");
            /* cleaning up */
            curl_easy_cleanup(curl_handle);
            curl_global_cleanup();


//            printf("Checking to see if it gets to before sem wait\n");
            /* Wait for stack to be not full then add to it*/
//            printf("Before sem wait is being called\n");
//            printf("CHeck to see if before sem wait\n");
            sem_wait(&shared_data_temp->spaces);
//            printf("CHeck to see if after sem wait\n");
//            printf("Checking to see if in sem_wait\n");
            pthread_mutex_lock(&shared_data_temp->stack_mutex);
//            char *item = (char *)p_shm_recv_buf + sizeof(RECV_BUF);
//            int *int_item = (int *) item;
            int *item = 1;
//            push(&shared_data_temp->pic_stack, *item);
            pthread_mutex_unlock(&shared_data_temp->stack_mutex);
            sem_post(&shared_data_temp->items);
//            printf("About to go up to top of while loop\n");
        } else{
            printf("Exiting loop for %i", n);
//            print_all_items_on_stack(shared_data_temp->pic_stack);
            break;
        }
    }
    return 0;
}

/* pop STACK_SIZE items off the stack */
void pop_all(struct int_stack *p)
{
    int i;
    if ( p == NULL) {
        abort();
    }

    for ( i = 0; ; i++ ) {
        int item;
        int ret = pop(p, &item);
        if ( ret != 0 ) {
            break;
        }
        printf("item[%d] = 0x%4X popped\n", i, item);
    }

    printf("%d items popped off the stack.\n", i);

}

int main()
{
    int i=0;
    pid_t pid=0;
    pid_t cpids[NUM_CHILD];
    int state;
    double times[2];
    struct timeval tv;
    int shm_size = sizeof_shm_stack(STACK_SIZE);

    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

    shared_data s_data;

    pthread_mutexattr_t tempmutex;
    pthread_mutexattr_init(&tempmutex);
    pthread_mutexattr_setpshared(&tempmutex, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&s_data.p_mutex, &tempmutex);

    pthread_mutexattr_t tempmutex2;
    pthread_mutexattr_init(&tempmutex2);
    pthread_mutexattr_setpshared(&tempmutex2, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&s_data.c_mutex, &tempmutex2);

    pthread_mutexattr_t tempmutex3;
    pthread_mutexattr_init(&tempmutex3);
    pthread_mutexattr_setpshared(&tempmutex3, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&s_data.stack_mutex, &tempmutex3);

//    sem_t spaces;
//    sem_t items;
//
//    if ( sem_init(&spaces, 1, STACK_SIZE) != 0 ) {
//        perror("sem_init(sem[0])");
//        abort();
//    }
//    if ( sem_init(&items, 1, 0) != 0 ) {
//        perror("sem_init(sem[1])");
//        abort();
//    }
//
//    s_data.spaces = spaces;
//    s_data.items = items;

//    struct int_stack *pstack;
////    memset(pstack, 0, sizeof(pstack));
//    init_shm_stack(pstack, STACK_SIZE);
//
//    s_data.pic_stack = &pstack;
//
//    struct int_stack *num_stack;
////    memset(num_stack, 0, sizeof(num_stack));
//    init_shm_stack(num_stack, STACK_SIZE);

//    s_data.pic_int_stack = &num_stack;

    int p_strip_num = 0;
    s_data.p_num_strip = &p_strip_num;

    s_data.p_num_strip = 0;

    int c_strip_num = 0;
    s_data.c_num_strip = &c_strip_num;
    s_data.c_num_strip = 0;


    if ( sem_init(&s_data.spaces, 1, STACK_SIZE) != 0 ) {
        perror("sem_init(sem[0])");
        abort();
    }
    if ( sem_init(&s_data.items, 1, 0) != 0 ) {
        perror("sem_init(sem[1])");
        abort();
    }

    int value;
    sem_getvalue(&s_data.spaces, &value);
    printf("The value of my semaphore is %i (before fork) \n", value);

    int shmid = shmget(IPC_PRIVATE, sizeof(shared_data), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

    int value2;
    sem_getvalue(&s_data.spaces, &value2);
    printf("The value of my semaphore is %i (before fork) \n", value2);

    shared_data *s_data2;
    s_data2 = shmat(shmid, NULL, 0);

    if ( sem_init(&s_data2->spaces, 1, STACK_SIZE) != 0 ) {
        perror("sem_init(sem[0])");
        abort();
    }
    if ( sem_init(&s_data2->items, 1, 0) != 0 ) {
        perror("sem_init(sem[1])");
        abort();
    }

    pthread_mutexattr_t newtempmutex;
    pthread_mutexattr_init(&newtempmutex);
    pthread_mutexattr_setpshared(&newtempmutex, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&s_data.p_mutex, &newtempmutex);

//    pthread_mutexattr_t newtempmutex2;
//    pthread_mutexattr_init(&newtempmutex2);
//    pthread_mutexattr_setpshared(&newtempmutex2, PTHREAD_PROCESS_SHARED);
//    pthread_mutex_init(&s_data.c_mutex, &newtempmutex2);
//
//    pthread_mutexattr_t newtempmutex3;
//    pthread_mutexattr_init(&newtempmutex3);
//    pthread_mutexattr_setpshared(&newtempmutex3, PTHREAD_PROCESS_SHARED);
//    pthread_mutex_init(&s_data.stack_mutex, &newtempmutex3);

    ISTACK pstack;
//    memset(pstack, 0, sizeof(pstack));
    init_shm_stack(&pstack, STACK_SIZE);

    s_data.pic_stack = pstack;

    ISTACK num_stack;
//    memset(num_stack, 0, sizeof(num_stack));
    init_shm_stack(&num_stack, STACK_SIZE);

    s_data.pic_int_stack = num_stack;

    int value3;
    sem_getvalue(&s_data2->spaces, &value3);
    printf("The value of my semaphore is %i (before fork) \n", value3);
//    s_data = shmat(shmid, NULL, 0);
    
    for ( i = 0; i < NUM_CHILD; i++) {
        
        pid = fork();

        if ( pid > 0 ) {        /* parent proc */
            cpids[i] = pid;
//            waitpid(cpids[i], &state, 0);
//            printf("I am the parent and I am done cycle %i\n", i);
        } else if ( pid == 0 ) { /* child proc */
            produce(i, shmid, s_data2);
            break;
        } else {
            perror("fork");
            abort();
        }
        
    }

    if ( pid > 0){
        for (i = 0 ; i < NUM_CHILD ; i++){
            waitpid(cpids[i], &state, 0);
            printf("Child %i is done! Woohoo\n", i);
        }
    }

//    for ( i = 0; i < NUM_CHILD; i++) {
//
//        pid = fork();
//
//        if ( pid > 0 ) {        /* parent proc */
//            int state;
//            waitpid(pid, &state, 0);
//        } else if ( pid == 0 ) { /* child proc */
//            consume(i, shmid);
//            break;
//        } else {
//            perror("fork");
//            abort();
//        }
//
//    }


//    if ( shmctl(shmid, IPC_RMID, NULL) == -1 ) {
//        perror("shmctl");
//        abort();
//    }

    return 0;
}
