#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include "a2_helper.h"

sem_t s_p5t4_start, s_p5t3_end;

void *p5_thread_routine(void *arg)
{

    int tid = *(int *)arg;

    if (tid == 3)
    {
        sem_wait(&s_p5t4_start);
        info(BEGIN, 5, tid);
        info(END, 5, tid);
        sem_post(&s_p5t3_end);
    }
    else if (tid == 4)
    {
        info(BEGIN, 5, 4);
        sem_post(&s_p5t4_start);
        sem_wait(&s_p5t3_end);
        info(END, 5, 4);
    }
    else
    {
        info(BEGIN, 5, tid);
        info(END, 5, tid);
    }

    return NULL;
}

void create_process5_threads()
{
    pthread_t p5_threads[4];
    int tids[4] = {1, 2, 3, 4};

    sem_init(&s_p5t4_start, 0, 0);
    sem_init(&s_p5t3_end, 0, 0);

    for (int tz = 0; tz < 4; tz++)
    {
        pthread_create(&p5_threads[tz], NULL, p5_thread_routine, &tids[tz]);
    }

    for (int tz = 0; tz < 4; tz++)
    {
        pthread_join(p5_threads[tz], NULL);
    }

    sem_destroy(&s_p5t4_start);
    sem_destroy(&s_p5t3_end);
}

void create_processes()
{
    pid_t p2, p3, p4, p5, p6, p7, p8, p9;

    /*

    p1 -> p2, p3, p5, p9
        p2 -> p7
        p3 -> p4
        p5 -> p6, p8
        p9 -> nothing
    */

    p2 = fork();

    if (p2 == 0)
    {
        info(BEGIN, 2, 0);

        p7 = fork();

        if (p7 == 0)
        {
            info(BEGIN, 7, 0);
            info(END, 7, 0);
            exit(0);
        }

        wait(NULL);
        info(END, 2, 0);
        exit(0);
    }

    p3 = fork();

    if (p3 == 0)
    {
        info(BEGIN, 3, 0);

        p4 = fork();
        if (p4 == 0)
        {
            info(BEGIN, 4, 0);
            info(END, 4, 0);
            exit(0);
        }

        wait(NULL);
        info(END, 3, 0);
        exit(0);
    }

    p5 = fork();

    if (p5 == 0)
    {
        info(BEGIN, 5, 0);

        p6 = fork();

        if (p6 == 0)
        {
            info(BEGIN, 6, 0);
            info(END, 6, 0);
            exit(0);
        }

        p8 = fork();

        if (p8 == 0)
        {
            info(BEGIN, 8, 0);
            info(END, 8, 0);
            exit(0);
        }

        create_process5_threads();

        wait(NULL);
        wait(NULL);

        info(END, 5, 0);
        exit(0);
    }

    p9 = fork();
    if (p9 == 0)
    {
        info(BEGIN, 9, 0);
        info(END, 9, 0);
        exit(0);
    }

    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);

    // exit(0);
}

// gcc -Wall a2.c a2_helper.c -o a2 -pthread -lrt
int main()
{
    init();

    info(BEGIN, 1, 0);

    create_processes();

    info(END, 1, 0);
    return 0;
}