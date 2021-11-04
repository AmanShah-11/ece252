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


#define SHM_SIZE 256
#define NUM_CHILD 6

#define STACK_SIZE 50
void push_all(struct int_stack *p, int start);
void pop_all(struct int_stack *p);
void test_local();
void test_shm();


typedef struct {
    struct int_stack *pic_stack;
    struct int_stack *pic_int_stack;
    int p_num_strip;
    sem_t spaces;
    sem_t items;
    pthread_mutex_t p_mutex;
    pthread_mutex_t c_mutex;
    pthread_mutex_t stack_mutex;
} shared_data;

int consume(int n, int shmid);
int produce(int n, int shmid);

int consume(int n, int shmid)
{
//    struct int_stack *pstack;
//    pstack = shmat(shmid, NULL, 0);
//    if ( pstack == (void *) -1 ) {
//        perror("shmat");
//        abort();
//    }
//    int temp = n;
//    push(pstack, temp);
//    printf("child: pstack = %p\n", pstack);
//    usleep((n+1)*1000);
//    printf("Worker ID=%d, pid = %d, ppid = %d.\n", n, getpid(), getppid());
//
//    if ( shmdt(pstack) != 0 ) {
//        perror("shmdt");
//        abort();
//    }
//
//    return 0;
    printf("I am the worker number %i with process %i\n", n, shmid);
    return 0;
}

int produce(int n, int shmid){
    /* Get shared data */
    shared_data *shared_data_temp;
    shared_data_temp = shmat(shmid, NULL, 0);
//    shared_data *shared_data_temp = shmat(shmid, NULL, 0);
    if ( shared_data_temp == (void *) -1 ) {
        perror("shmat");
        abort();
    }

    int value;
    sem_getvalue(&shared_data_temp->spaces, &value);
    printf("The value of my semaphore is %i (in produce) \n", value);


    /* Infinite while loop until we finish all 50 strips*/
    while(1){
        printf("In while loop\n");
        int produce_count;
        /* Critical section for obtaining strip num */
        pthread_mutex_lock(&shared_data_temp->p_mutex);
        shared_data_temp->p_num_strip += 1;
        produce_count = shared_data_temp->p_num_strip;
        pthread_mutex_unlock(&shared_data_temp->p_mutex);

        if (produce_count <= 5){
            /* Make cURL call to server */
            printf("Checking to see if it gets to before sem wait\n");
            /* Wait for stack to be not full then add to it*/
            sem_wait(&shared_data_temp->spaces);
            printf("Checking to see if in sem_wait\n");
            pthread_mutex_lock(&shared_data_temp->stack_mutex);
            push(shared_data_temp->pic_stack, produce_count);
            pthread_mutex_unlock(&shared_data_temp->stack_mutex);
            sem_post(&shared_data_temp->items);

        } else{
            print_all_items_on_stack(shared_data_temp->pic_stack);
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

    struct int_stack *pstack;
    init_shm_stack(pstack, STACK_SIZE);

    s_data.pic_stack = &pstack;

    struct int_stack *num_stack;
    init_shm_stack(num_stack, STACK_SIZE);

    s_data.pic_int_stack = &num_stack;

    int p_strip_num = 0;
    s_data.p_num_strip = &p_strip_num;

    s_data.p_num_strip = 0;

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

//    s_data = shmat(shmid, NULL, 0);
    
    for ( i = 0; i < NUM_CHILD; i++) {
        
        pid = fork();

        if ( pid > 0 ) {        /* parent proc */
            cpids[i] = pid;
//            waitpid(cpids[i], &state, 0);
//            printf("I am the parent and I am done cycle %i\n", i);
        } else if ( pid == 0 ) { /* child proc */
            produce(i, shmid);
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
