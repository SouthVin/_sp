#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

static void demonstrate_basic_fork() {
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
        return;
    }

    if (pid == 0) {
        printf("[CHILD]  PID=%d, Parent PID=%d, fork() returned %d\n",
               getpid(), getppid(), pid);
        _exit(0);
    }

    printf("[PARENT] PID=%d, Child PID=%d, fork() returned %d\n",
           getpid(), pid, pid);

    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        printf("[PARENT] Child exited with status %d\n\n", WEXITSTATUS(status));
    }
}

static void demonstrate_variable_separation() {
    int shared = 42;

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
        return;
    }

    if (pid == 0) {
        shared = 99;
        printf("[CHILD]  Modified shared to %d (address: %p)\n", shared, (void*)&shared);
        _exit(0);
    }

    int status;
    waitpid(pid, &status, 0);
    printf("[PARENT] shared is still %d (address: %p)\n", shared, (void*)&shared);
    printf("[PARENT] Child and parent have SEPARATE address spaces!\n\n");
}

static void demonstrate_multiple_children(int n) {
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            fprintf(stderr, "fork failed at child %d: %s\n", i, strerror(errno));
            break;
        }

        if (pid == 0) {
            printf("[CHILD %d] PID=%d, doing some work...\n", i + 1, getpid());
            sleep(1);
            _exit(i + 10);
        }

        printf("[PARENT] Launched child %d (PID=%d)\n", i + 1, pid);
    }

    for (int i = 0; i < n; i++) {
        int status;
        pid_t pid = wait(&status);
        if (WIFEXITED(status)) {
            printf("[PARENT] Child %d finished (exit code %d)\n",
                   (int)pid, WEXITSTATUS(status));
        }
    }
    printf("\n");
}

static void demonstrate_orphan() {
    printf("--- Orphan Process Demo (child sleeps, parent exits early) ---\n");
    printf("    Run 'ps -ef | grep orphan' in another terminal to see the orphan\n");

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
        return;
    }

    if (pid == 0) {
        printf("[ORPHAN-DEMO] Child PID=%d, Parent PID=%d\n", getpid(), getppid());
        printf("[ORPHAN-DEMO] Sleeping 3 seconds — parent will exit after 1s...\n");
        sleep(3);
        printf("[ORPHAN-DEMO] I woke up! My new parent PID is %d (was %d before)\n",
               getppid(), (int)getppid());
        printf("[ORPHAN-DEMO] Note: if new PPID is 1, systemd/init adopted me\n\n");
        _exit(0);
    }

    printf("[PARENT]  I will exit in 1 second, leaving my child an orphan\n");
    sleep(1);
    printf("[PARENT]  Exiting now...\n");
    _exit(0);
}

int main() {
    printf("=== 01-fork: Process Creation with fork() ===\n\n");

    printf("--- Basic fork() ---\n");
    demonstrate_basic_fork();

    printf("--- Variable Separation (proof of separate address spaces) ---\n");
    demonstrate_variable_separation();

    printf("--- Multiple Children (fork in a loop) ---\n");
    demonstrate_multiple_children(3);

    demonstrate_orphan();

    printf("=== Done ===\n");
    return 0;
}
