#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 10
#define ITEMS_TO_PRODUCE 30

int buffer[BUFFER_SIZE];
int count = 0;
int in = 0;
int out = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

void *producer(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < ITEMS_TO_PRODUCE; i++) {
        int item = (id * 1000) + i;

        pthread_mutex_lock(&mutex);
        while (count == BUFFER_SIZE) {
            printf("[Producer %d] Buffer full, waiting...\n", id);
            pthread_cond_wait(&not_full, &mutex);
        }

        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        printf("[Producer %d] Produced item %d  (buffer: %d/%d)\n", id, item, count, BUFFER_SIZE);

        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&mutex);

        usleep(10000);
    }
    return NULL;
}

void *consumer(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < ITEMS_TO_PRODUCE; i++) {
        pthread_mutex_lock(&mutex);
        while (count == 0) {
            printf("[Consumer %d] Buffer empty, waiting...\n", id);
            pthread_cond_wait(&not_empty, &mutex);
        }

        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;
        printf("[Consumer %d] Consumed item %d  (buffer: %d/%d)\n", id, item, count, BUFFER_SIZE);

        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&mutex);

        usleep(15000);
    }
    return NULL;
}

int main() {
    pthread_t prod[2], cons[2];
    int prod_ids[2] = {1, 2};
    int cons_ids[2] = {1, 2};

    printf("=== Producer-Consumer Simulation ===\n");
    printf("Buffer size: %d | Items per producer: %d | Producers: 2 | Consumers: 2\n\n", BUFFER_SIZE, ITEMS_TO_PRODUCE);

    for (int i = 0; i < 2; i++) {
        pthread_create(&prod[i], NULL, producer, &prod_ids[i]);
        pthread_create(&cons[i], NULL, consumer, &cons_ids[i]);
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(prod[i], NULL);
        pthread_join(cons[i], NULL);
    }

    printf("\n=== Simulation Complete ===\n");
    printf("Final buffer count: %d (expected: 0)\n", count);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_full);
    pthread_cond_destroy(&not_empty);
    return 0;
}
