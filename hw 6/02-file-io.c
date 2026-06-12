#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096
#define TEST_FILE "02-test-output.txt"

static void demo_write_to_file() {
    int fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "open(%s) failed: %s\n", TEST_FILE, strerror(errno));
        return;
    }

    const char *lines[] = {
        "Line 1: Hello from 02-file-io!\n",
        "Line 2: This demonstrates write().\n",
        "Line 3: Each byte goes through the kernel.\n",
    };

    for (int i = 0; i < 3; i++) {
        ssize_t written = write(fd, lines[i], strlen(lines[i]));
        if (written < 0) {
            fprintf(stderr, "write failed: %s\n", strerror(errno));
            close(fd);
            return;
        }
        printf("  Wrote %zd bytes: %s", written, lines[i]);
    }

    close(fd);
    printf("  File '%s' created successfully.\n\n", TEST_FILE);
}

static void demo_read_file() {
    int fd = open(TEST_FILE, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open(%s) for reading failed: %s\n", TEST_FILE, strerror(errno));
        return;
    }

    char buf[BUFFER_SIZE];
    ssize_t bytes_read = read(fd, buf, sizeof(buf) - 1);
    if (bytes_read < 0) {
        fprintf(stderr, "read failed: %s\n", strerror(errno));
        close(fd);
        return;
    }

    buf[bytes_read] = '\0';
    close(fd);

    printf("  Read %zd bytes from '%s':\n", bytes_read, TEST_FILE);
    printf("  --- start of file ---\n%s  --- end of file ---\n\n", buf);
}

static void demo_lseek() {
    int fd = open(TEST_FILE, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open failed: %s\n", strerror(errno));
        return;
    }

    off_t size = lseek(fd, 0, SEEK_END);
    printf("  File size: %ld bytes\n", (long)size);

    lseek(fd, 0, SEEK_SET);
    char first_line[128];
    ssize_t n = read(fd, first_line, sizeof(first_line) - 1);
    first_line[n] = '\0';

    char *nl = strchr(first_line, '\n');
    if (nl) *nl = '\0';
    printf("  First line: \"%s\"\n", first_line);

    close(fd);
    printf("\n");
}

static void demo_read_from_stdin() {
    printf("  Please type something and press Enter: ");
    fflush(stdout);

    char buf[256];
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
    if (n < 0) {
        fprintf(stderr, "read from stdin failed: %s\n", strerror(errno));
        return;
    }

    buf[n] = '\0';
    char *nl = strchr(buf, '\n');
    if (nl) *nl = '\0';

    printf("  You typed (read via fd 0): \"%s\"\n\n", buf);
}

static void demo_stdout_vs_stderr() {
    printf("  [1] This goes to stdout (fd 1)\n");
    fprintf(stderr, "  [2] This goes to stderr (fd 2)\n");

    write(STDOUT_FILENO, "  [3] write() to fd 1\n", 21);
    write(STDERR_FILENO, "  [4] write() to fd 2\n", 21);
    printf("\n");
}

static void demo_copy_file() {
    const char *copy_name = "02-copy.txt";
    int src = open(TEST_FILE, O_RDONLY);
    if (src < 0) {
        fprintf(stderr, "open src failed: %s\n", strerror(errno));
        return;
    }

    int dst = open(copy_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst < 0) {
        fprintf(stderr, "open dst failed: %s\n", strerror(errno));
        close(src);
        return;
    }

    char buf[BUFFER_SIZE];
    ssize_t n;
    size_t total = 0;

    while ((n = read(src, buf, sizeof(buf))) > 0) {
        ssize_t written = write(dst, buf, n);
        if (written < 0) {
            fprintf(stderr, "write during copy failed: %s\n", strerror(errno));
            break;
        }
        total += written;
    }

    close(src);
    close(dst);

    printf("  Copied %zu bytes from '%s' to '%s'\n\n", total, TEST_FILE, copy_name);
}

int main() {
    printf("=== 02-file-io: File Descriptors and I/O ===\n\n");

    printf("--- open() + write() — Creating a file ---\n");
    demo_write_to_file();

    printf("--- open() + read() — Reading a file ---\n");
    demo_read_file();

    printf("--- lseek() — Seeking in a file ---\n");
    demo_lseek();

    printf("--- read() from stdin (fd 0) ---\n");
    demo_read_from_stdin();

    printf("--- stdout (fd 1) vs stderr (fd 2) ---\n");
    demo_stdout_vs_stderr();

    printf("--- copy: read() loop + write() ---\n");
    demo_copy_file();

    printf("=== Done ===\n");
    return 0;
}
