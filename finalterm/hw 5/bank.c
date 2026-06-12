#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define OPERATIONS 100000

int balance = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *deposit(void *arg) {
    (void)arg;
    for (int i = 0; i < OPERATIONS; i++) {
        pthread_mutex_lock(&lock);
        balance = balance + 1;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void *withdraw(void *arg) {
    (void)arg;
    for (int i = 0; i < OPERATIONS; i++) {
        pthread_mutex_lock(&lock);
        balance = balance - 1;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, deposit, NULL);
    pthread_create(&t2, NULL, withdraw, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Final balance: %d\n", balance);

    if (balance == 0) {
        printf("SUCCESS: No race condition detected.\n");
    } else {
        printf("FAIL: Race condition corrupted the balance!\n");
    }

    pthread_mutex_destroy(&lock);
    return 0;
}
