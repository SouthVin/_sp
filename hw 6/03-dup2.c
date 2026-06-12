#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

/*
 * dup2(oldfd, newfd):
 *   - Makes newfd refer to the same open file as oldfd.
 *   - If newfd was open, it is silently closed first.
 *   - Returns newfd on success, -1 on error.
 *
 * This is how shells implement redirection:
 *   command > file   =>  dup2(fd_file, STDOUT_FILENO)
 *   command < file   =>  dup2(fd_file, STDIN_FILENO)
 *   2>&1             =>  dup2(STDOUT_FILENO, STDERR_FILENO)
 */

static void demo_stdout_redirect_to_file() {
    printf("--- Redirecting stdout to a file (like `command > file`) ---\n");

    int saved_stdout = dup(STDOUT_FILENO);
    if (saved_stdout < 0) {
        fprintf(stderr, "dup failed: %s\n", strerror(errno));
        return;
    }

    int fd = open("03-stdout-redirect.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "open failed: %s\n", strerror(errno));
        close(saved_stdout);
        return;
    }

    dup2(fd, STDOUT_FILENO);
    close(fd);

    printf("This line goes into the FILE, not the terminal!\n");
    printf("So does this one.\n");
    fflush(stdout);

    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);

    printf("  stdout restored. Check '03-stdout-redirect.txt'\n\n");
}

static void demo_stderr_redirect() {
    printf("--- Redirecting stderr to a file (like `command 2> file`) ---\n");

    int saved_stderr = dup(STDERR_FILENO);
    if (saved_stderr < 0) {
        fprintf(stderr, "dup failed: %s\n", strerror(errno));
        return;
    }

    int fd = open("03-stderr-redirect.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "open failed: %s\n", strerror(errno));
        close(saved_stderr);
        return;
    }

    dup2(fd, STDERR_FILENO);
    close(fd);

    fprintf(stderr, "This stderr message goes to the file!\n");
    fflush(stderr);

    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stderr);

    printf("  stderr restored. Check '03-stderr-redirect.txt'\n\n");
}

static void demo_merge_stderr_into_stdout() {
    printf("--- Merging stderr into stdout (like `command 2>&1`) ---\n");

    int saved_stderr = dup(STDERR_FILENO);
    dup2(STDOUT_FILENO, STDERR_FILENO);

    printf("  Hello from stdout\n");
    fprintf(stderr, "  Hello from stderr (redirected to stdout)\n");
    fflush(stdout);
    fflush(stderr);

    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stderr);

    printf("  stderr restored.\n\n");
}

static void demo_fork_exec_redirect() {
    printf("--- Fork + execvp with stdout redirect (like `ls -la > file`) ---\n");

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
        return;
    }

    if (pid == 0) {
        int fd = open("03-ls-output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open");
            _exit(1);
        }

        dup2(fd, STDOUT_FILENO);
        close(fd);

        char *args[] = {"ls", "-la", "..", NULL};
        execvp("ls", args);
        perror("execvp failed");
        _exit(1);
    }

    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("  ls output written to '03-ls-output.txt'\n");
    } else {
        printf("  Command failed\n");
    }
    printf("\n");
}

static void demo_pipe_simulation_with_fork_dup2() {
    printf("--- Simulating a pipe: child writes, parent reads via dup2 + fork ---\n");

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        fprintf(stderr, "pipe failed: %s\n", strerror(errno));
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
        return;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execlp("echo", "echo", "Hello through the pipe!", (char *)NULL);
        perror("execlp failed");
        _exit(1);
    }

    close(pipefd[1]);

    char buf[256];
    ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0';
        printf("  Parent received: \"%s\"\n", buf);
    }

    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);
    printf("\n");
}

int main() {
    printf("=== 03-dup2: File Descriptor Duplication and Redirection ===\n\n");

    demo_stdout_redirect_to_file();

    demo_stderr_redirect();

    demo_merge_stderr_into_stdout();

    demo_fork_exec_redirect();

    demo_pipe_simulation_with_fork_dup2();

    printf("=== Done ===\n");
    return 0;
}
