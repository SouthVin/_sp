#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define N 5
#define EAT_COUNT 3

pthread_mutex_t forks[N];

void think(int id) {
    printf("[Philosopher %d] Thinking...\n", id);
    usleep(100000 + (rand() % 200000));
}

void eat(int id) {
    printf("[Philosopher %d] Eating...\n", id);
    usleep(100000 + (rand() % 200000));
}

void *philosopher(void *arg) {
    int id = *(int *)arg;
    int left = id;
    int right = (id + 1) % N;

    int first, second;
    if (left < right) {
        first = left;
        second = right;
    } else {
        first = right;
        second = left;
    }

    for (int i = 0; i < EAT_COUNT; i++) {
        think(id);

        printf("[Philosopher %d] Hungry, picking up forks...\n", id);

        pthread_mutex_lock(&forks[first]);
        printf("[Philosopher %d] Picked up fork %d\n", id, first);
        pthread_mutex_lock(&forks[second]);
        printf("[Philosopher %d] Picked up fork %d\n", id, second);

        eat(id);

        pthread_mutex_unlock(&forks[second]);
        printf("[Philosopher %d] Put down fork %d\n", id, second);
        pthread_mutex_unlock(&forks[first]);
        printf("[Philosopher %d] Put down fork %d\n", id, first);
    }

    printf("[Philosopher %d] Finished all meals.\n", id);
    return NULL;
}

int main() {
    pthread_t philosophers[N];
    int ids[N];

    srand(42);

    printf("=== Dining Philosophers Simulation ===\n");
    printf("Philosophers: %d | Meals per philosopher: %d\n", N, EAT_COUNT);
    printf("Deadlock prevention: Resource hierarchy (lower fork first)\n\n");

    for (int i = 0; i < N; i++) {
        pthread_mutex_init(&forks[i], NULL);
    }

    for (int i = 0; i < N; i++) {
        ids[i] = i;
        pthread_create(&philosophers[i], NULL, philosopher, &ids[i]);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(philosophers[i], NULL);
    }

    printf("\n=== All philosophers finished — No deadlock! ===\n");

    for (int i = 0; i < N; i++) {
        pthread_mutex_destroy(&forks[i]);
    }

    return 0;
}
