#include "toyemu.h"
#include <stdio.h>
#include <stdlib.h>

void assemble_file(const char *filename, uint8_t **out, int *out_len);

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.asm>\n", argv[0]);
        return 1;
    }

    uint8_t *prog;
    int      len;

    assemble_file(argv[1], &prog, &len);

    printf("[ASSEMBLER] %d bytes generated\n", len);

    CPU cpu;
    cpu_init(&cpu);
    cpu_load(&cpu, prog, len);
    free(prog);

    printf("[CPU] Starting execution...\n");
    cpu_run(&cpu);
    printf("[CPU] Halted.\n");

    return 0;
}
