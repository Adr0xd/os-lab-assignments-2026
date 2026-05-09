#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include "a2_helper.h"

sem_t s_p5t4_start, s_p5t3_end;

sem_t *s_p5t1_end, *s_p4t3_end;

sem_t s_p2_6max;
pthread_mutex_t p2t13_mutex;
int active = 0;
pthread_cond_t p2t13_cond;

int t13_start, t13_end;

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
    else if (tid == 1)
    {
        sem_wait(s_p4t3_end);
        info(BEGIN, 5, 1);
        info(END, 5, 1);
        sem_post(s_p5t1_end);
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

void *p2_thread_routine(void *arg)
{
    int tid = *(int *)arg;
    sem_wait(&s_p2_6max);

    pthread_mutex_lock(&p2t13_mutex);

    active++;
    if (tid == 13)
        t13_start = 1;
    pthread_cond_broadcast(&p2t13_cond);

    pthread_mutex_unlock(&p2t13_mutex);

    info(BEGIN, 2, tid);

    if (tid == 13)
    {
        pthread_mutex_lock(&p2t13_mutex);

        while (active != 6)
        {
            pthread_cond_wait(&p2t13_cond, &p2t13_mutex);
        }
        pthread_cond_broadcast(&p2t13_cond);

        pthread_mutex_unlock(&p2t13_mutex);
    }
    else
    {
        pthread_mutex_lock(&p2t13_mutex);

        while (t13_start == 1 && t13_end == 0)
        {
            // if 13 has started but not yet finished
            // make the other ones wait until 13 ends
            pthread_cond_wait(&p2t13_cond, &p2t13_mutex);
        }

        pthread_mutex_unlock(&p2t13_mutex);
    }

    info(END, 2, tid);

    if (tid == 13)
    {
        pthread_mutex_lock(&p2t13_mutex);
        t13_end = 1;
        pthread_cond_broadcast(&p2t13_cond);
        pthread_mutex_unlock(&p2t13_mutex);
    }

    pthread_mutex_lock(&p2t13_mutex);

    active--;
    pthread_cond_broadcast(&p2t13_cond);

    pthread_mutex_unlock(&p2t13_mutex);

    sem_post(&s_p2_6max);
    return NULL;
}

void create_process2_threads()
{

    pthread_t p2_threads[39];
    int tids[39];
    for (int sh = 0; sh < 39; sh++)
    {
        tids[sh] = sh + 1;
    }

    sem_init(&s_p2_6max, 0, 6);
    pthread_mutex_init(&p2t13_mutex, NULL);
    pthread_cond_init(&p2t13_cond, NULL);

    t13_start = t13_end = active = 0;

    for (int tz = 0; tz < 39; tz++)
    {
        pthread_create(&p2_threads[tz], NULL, p2_thread_routine, &tids[tz]);
    }

    for (int tz = 0; tz < 39; tz++)
    {
        pthread_join(p2_threads[tz], NULL);
    }

    sem_destroy(&s_p2_6max);
    pthread_mutex_destroy(&p2t13_mutex);
    pthread_cond_destroy(&p2t13_cond);
}

void *p4_thread_routine(void *arg)
{

    int tid = *(int *)arg;

    if (tid == 3)
    {
        info(BEGIN, 4, 3);
        info(END, 4, 3);
        sem_post(s_p4t3_end);
    }
    else if (tid == 5)
    {
        sem_wait(s_p5t1_end);
        info(BEGIN, 4, 5);
        info(END, 4, 5);
    }
    else
    {
        info(BEGIN, 4, tid);
        info(END, 4, tid);
    }

    return NULL;
}

void create_process4_threads()
{

    pthread_t p4_threads[6];
    int tids[6];
    for (int sh = 0; sh < 6; sh++)
    {
        tids[sh] = sh + 1;
    }

    for (int tz = 0; tz < 6; tz++)
    {
        pthread_create(&p4_threads[tz], NULL, p4_thread_routine, &tids[tz]);
    }

    for (int tz = 0; tz < 6; tz++)
    {
        pthread_join(p4_threads[tz], NULL);
    }
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

        create_process2_threads();

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

            create_process4_threads();

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
    sem_unlink("/s_p4t3_end");
    sem_unlink("/s_p5t1_end");

    s_p5t1_end = sem_open("/s_p5t1_end", O_CREAT, 0644, 0);
    s_p4t3_end = sem_open("/s_p4t3_end", O_CREAT, 0644, 0);

    init();

    info(BEGIN, 1, 0);

    create_processes();

    info(END, 1, 0);

    sem_close(s_p4t3_end);
    sem_close(s_p5t1_end);

    sem_unlink("/s_p4t3_end");
    sem_unlink("/s_p5t1_end");

    return 0;
}
