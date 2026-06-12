#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_TOKENS 128

/*
 * Tokenize a command line into an argv-style array.
 * Handles spaces, tabs, and quotes (single and double).
 * Returns the number of tokens.
 */
static int tokenize(char *line, char *tokens[], int max_tokens) {
    int count = 0;
    char *p = line;

    while (*p && count < max_tokens) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        if (*p == '"' || *p == '\'') {
            char quote = *p++;
            tokens[count++] = p;
            while (*p && *p != quote) p++;
            if (*p == quote) *p++ = '\0';
        } else {
            tokens[count++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = '\0';
        }
    }

    tokens[count] = NULL;
    return count;
}

/*
 * Split tokens by redirection operators (<, >).
 * The 'args' array gets the command and its arguments.
 * 'input_file' and 'output_file' are set if redirection is found.
 * Returns 0 on success.
 */
static int parse_redirect(char *tokens[], char *args[], int max_args,
                          char **input_file, char **output_file) {
    int arg_count = 0;
    *input_file = NULL;
    *output_file = NULL;

    for (int i = 0; tokens[i] != NULL && arg_count < max_args; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            i++;
            if (tokens[i] != NULL) {
                *input_file = tokens[i];
            }
        } else if (strcmp(tokens[i], ">") == 0) {
            i++;
            if (tokens[i] != NULL) {
                *output_file = tokens[i];
            }
        } else {
            args[arg_count++] = tokens[i];
        }
    }

    args[arg_count] = NULL;
    return arg_count;
}

static void execute_cmd(char *args[], char *input_file, char *output_file) {
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "shell: fork failed: %s\n", strerror(errno));
        return;
    }

    if (pid == 0) {
        if (input_file != NULL) {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "shell: cannot open '%s': %s\n",
                        input_file, strerror(errno));
                _exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        if (output_file != NULL) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                fprintf(stderr, "shell: cannot open '%s': %s\n",
                        output_file, strerror(errno));
                _exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        execvp(args[0], args);
        fprintf(stderr, "shell: '%s': %s\n", args[0], strerror(errno));
        _exit(1);
    }

    int status;
    waitpid(pid, &status, 0);
}

static int builtin_cd(char *args[]) {
    if (args[1] == NULL) {
        fprintf(stderr, "shell: cd: expected argument\n");
        return 1;
    }
    if (chdir(args[1]) < 0) {
        fprintf(stderr, "shell: cd: %s: %s\n", args[1], strerror(errno));
        return 1;
    }
    return 0;
}

int main() {
    char line[MAX_INPUT];
    char *tokens[MAX_TOKENS];
    char *args[MAX_ARGS];
    char *input_file, *output_file;

    printf("=== Simple Shell (type 'exit' to quit) ===\n");
    printf("Supports: command args, < input, > output\n\n");

    while (1) {
        printf(">> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0') continue;

        int ntokens = tokenize(line, tokens, MAX_TOKENS);
        if (ntokens == 0) continue;

        parse_redirect(tokens, args, MAX_ARGS, &input_file, &output_file);

        if (args[0] == NULL) continue;

        if (strcmp(args[0], "exit") == 0) {
            printf("Goodbye!\n");
            break;
        }

        if (strcmp(args[0], "cd") == 0) {
            builtin_cd(args);
            continue;
        }

        execute_cmd(args, input_file, output_file);
    }

    return 0;
}
