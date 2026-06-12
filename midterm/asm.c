#include "toyemu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LABELS 64
#define MAX_CODE   256
#define MAX_LINE   256
#define MAX_PATCH  64

typedef struct { char name[32]; uint8_t addr; } Label;
typedef struct { uint8_t patch_pos; char name[32]; } Patch;

static Label  labels[MAX_LABELS];
static int    label_count;
static uint8_t code[MAX_CODE];
static int    code_len;
static Patch  patches[MAX_PATCH];
static int    patch_count;

static int find_label(const char *name) {
    for (int i = 0; i < label_count; i++)
        if (strcmp(labels[i].name, name) == 0) return i;
    return -1;
}

static void add_label(const char *name, uint8_t addr) {
    if (find_label(name) >= 0) {
        fprintf(stderr, "Error: duplicate label '%s'\n", name); exit(1);
    }
    strncpy(labels[label_count].name, name, 31);
    labels[label_count].name[31] = '\0';
    labels[label_count].addr = addr;
    label_count++;
}

static int parse_reg(const char *s) {
    if (strlen(s) == 2 && s[0] == 'R' && s[1] >= '0' && s[1] <= '3')
        return s[1] - '0';
    return -1;
}

static int parse_int(const char *s, uint8_t *val) {
    if (s[0] == '\'') {
        if (s[1] == '\\' && s[2] == 'n' && s[3] == '\'') { *val = '\n'; return 1; }
        if (s[1] && s[2] == '\'' && s[3] == '\0') { *val = (uint8_t)s[1]; return 1; }
        return 0;
    }
    char *end;
    long v = strtol(s, &end, 0);
    if (*end != '\0' || v < 0 || v > 255) return 0;
    *val = (uint8_t)v;
    return 1;
}

static void emit_byte(uint8_t b) {
    if (code_len >= MAX_CODE) { fprintf(stderr, "Code buffer overflow\n"); exit(1); }
    code[code_len++] = b;
}

static void emit_label_addr(const char *name) {
    int idx = find_label(name);
    if (idx >= 0) {
        emit_byte(labels[idx].addr);
    } else {
        if (patch_count >= MAX_PATCH) { fprintf(stderr, "Too many patches\n"); exit(1); }
        patches[patch_count].patch_pos = code_len;
        strncpy(patches[patch_count].name, name, 31);
        patches[patch_count].name[31] = '\0';
        patch_count++;
        emit_byte(0xFF);
    }
}

static void resolve_patches(void) {
    for (int i = 0; i < patch_count; i++) {
        int idx = find_label(patches[i].name);
        if (idx < 0) {
            fprintf(stderr, "Error: undefined label '%s'\n", patches[i].name);
            exit(1);
        }
        code[patches[i].patch_pos] = labels[idx].addr;
    }
}

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

static void strip_comment(char *s) {
    char *c = strchr(s, ';');
    if (c) *c = '\0';
}

/* Returns instruction byte count for pass-1 address calculation */
static int instr_size(const char *mnemonic) {
    if (!strcmp(mnemonic, "NOP") || !strcmp(mnemonic, "HLT") || !strcmp(mnemonic, "RET"))
        return 1;
    if (!strcmp(mnemonic, "MVI") || !strcmp(mnemonic, "LD") || !strcmp(mnemonic, "ST"))
        return 3;
    return 2;
}

void assemble_file(const char *filename, uint8_t **out, int *out_len) {
    FILE *f = fopen(filename, "r");
    if (!f) { fprintf(stderr, "Cannot open '%s'\n", filename); exit(1); }

    char raw[MAX_LINE];
    label_count = 0;
    code_len = 0;
    patch_count = 0;
    memset(labels, 0, sizeof(labels));
    memset(code, 0, sizeof(code));
    memset(patches, 0, sizeof(patches));

    /* Pass 1 */
    while (fgets(raw, MAX_LINE, f)) {
        strip_comment(raw);
        char *line = trim(raw);
        if (*line == '\0') continue;

        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char *lbl = trim(line);
            if (*lbl) add_label(lbl, code_len);
            char *after = trim(colon + 1);
            if (*after == '\0') continue;
            line = after;
        }

        char mnemonic[16] = {0};
        sscanf(line, "%15s", mnemonic);
        code_len += instr_size(mnemonic);
    }

    /* Pass 2 */
    rewind(f);
    code_len = 0;

    while (fgets(raw, MAX_LINE, f)) {
        strip_comment(raw);
        char *line = trim(raw);
        if (*line == '\0') continue;

        char *colon = strchr(line, ':');
        if (colon) {
            char *after = trim(colon + 1);
            if (*after == '\0') continue;
            line = after;
        }

        char mnemonic[16], a1[64], a2[64];
        memset(a1, 0, sizeof(a1));
        memset(a2, 0, sizeof(a2));

        int n = sscanf(line, "%15s %63[^,],%63s", mnemonic, a1, a2);
        if (n < 2) { n = sscanf(line, "%15s %63s", mnemonic, a1); }

        if (n < 1) continue;

        if (!strcmp(mnemonic, "NOP")) { emit_byte(OP_NOP); }
        else if (!strcmp(mnemonic, "HLT")) { emit_byte(OP_HLT); }
        else if (!strcmp(mnemonic, "RET")) { emit_byte(OP_RET); }
        else if (!strcmp(mnemonic, "MOV")) {
            int rd = parse_reg(a1), rs = parse_reg(a2);
            if (rd < 0 || rs < 0) { fprintf(stderr, "Bad registers for MOV\n"); exit(1); }
            emit_byte(OP_MOV); emit_byte((rd << 4) | rs);
        }
        else if (!strcmp(mnemonic, "ADD")) {
            int rd = parse_reg(a1), rs = parse_reg(a2);
            if (rd < 0 || rs < 0) { fprintf(stderr, "Bad registers for ADD\n"); exit(1); }
            emit_byte(OP_ADD); emit_byte((rd << 4) | rs);
        }
        else if (!strcmp(mnemonic, "SUB")) {
            int rd = parse_reg(a1), rs = parse_reg(a2);
            if (rd < 0 || rs < 0) { fprintf(stderr, "Bad registers for SUB\n"); exit(1); }
            emit_byte(OP_SUB); emit_byte((rd << 4) | rs);
        }
        else if (!strcmp(mnemonic, "CMP")) {
            int rd = parse_reg(a1), rs = parse_reg(a2);
            if (rd < 0 || rs < 0) { fprintf(stderr, "Bad registers for CMP\n"); exit(1); }
            emit_byte(OP_CMP); emit_byte((rd << 4) | rs);
        }
        else if (!strcmp(mnemonic, "AND")) {
            int rd = parse_reg(a1), rs = parse_reg(a2);
            if (rd < 0 || rs < 0) { fprintf(stderr, "Bad registers for AND\n"); exit(1); }
            emit_byte(OP_AND); emit_byte((rd << 4) | rs);
        }
        else if (!strcmp(mnemonic, "OR")) {
            int rd = parse_reg(a1), rs = parse_reg(a2);
            if (rd < 0 || rs < 0) { fprintf(stderr, "Bad registers for OR\n"); exit(1); }
            emit_byte(OP_OR); emit_byte((rd << 4) | rs);
        }
        else if (!strcmp(mnemonic, "XOR")) {
            int rd = parse_reg(a1), rs = parse_reg(a2);
            if (rd < 0 || rs < 0) { fprintf(stderr, "Bad registers for XOR\n"); exit(1); }
            emit_byte(OP_XOR); emit_byte((rd << 4) | rs);
        }
        else if (!strcmp(mnemonic, "INC")) {
            int rd = parse_reg(a1);
            if (rd < 0) { fprintf(stderr, "Bad register for INC\n"); exit(1); }
            emit_byte(OP_INC); emit_byte(rd);
        }
        else if (!strcmp(mnemonic, "DEC")) {
            int rd = parse_reg(a1);
            if (rd < 0) { fprintf(stderr, "Bad register for DEC\n"); exit(1); }
            emit_byte(OP_DEC); emit_byte(rd);
        }
        else if (!strcmp(mnemonic, "PUSH")) {
            int rs = parse_reg(a1);
            if (rs < 0) { fprintf(stderr, "Bad register for PUSH\n"); exit(1); }
            emit_byte(OP_PUSH); emit_byte(rs);
        }
        else if (!strcmp(mnemonic, "POP")) {
            int rd = parse_reg(a1);
            if (rd < 0) { fprintf(stderr, "Bad register for POP\n"); exit(1); }
            emit_byte(OP_POP); emit_byte(rd);
        }
        else if (!strcmp(mnemonic, "OUT")) {
            int rd = parse_reg(a1);
            if (rd < 0) { fprintf(stderr, "Bad register for OUT\n"); exit(1); }
            emit_byte(OP_OUT); emit_byte(rd);
        }
        else if (!strcmp(mnemonic, "OUTC")) {
            int rd = parse_reg(a1);
            if (rd < 0) { fprintf(stderr, "Bad register for OUTC\n"); exit(1); }
            emit_byte(OP_OUTC); emit_byte(rd);
        }
        else if (!strcmp(mnemonic, "JMP"))  { emit_byte(OP_JMP);  emit_label_addr(a1); }
        else if (!strcmp(mnemonic, "JZ"))   { emit_byte(OP_JZ);   emit_label_addr(a1); }
        else if (!strcmp(mnemonic, "JNZ"))  { emit_byte(OP_JNZ);  emit_label_addr(a1); }
        else if (!strcmp(mnemonic, "CALL")) { emit_byte(OP_CALL); emit_label_addr(a1); }
        else if (!strcmp(mnemonic, "MVI")) {
            int rd = parse_reg(a1);
            if (rd < 0) { fprintf(stderr, "Bad register for MVI\n"); exit(1); }
            uint8_t imm;
            if (!parse_int(a2, &imm)) { fprintf(stderr, "Bad immediate for MVI\n"); exit(1); }
            emit_byte(OP_MVI); emit_byte(rd); emit_byte(imm);
        }
        else if (!strcmp(mnemonic, "LD")) {
            int rd = parse_reg(a1);
            if (rd < 0) { fprintf(stderr, "Bad register for LD\n"); exit(1); }
            uint8_t addr;
            if (!parse_int(a2, &addr)) { fprintf(stderr, "Bad address for LD\n"); exit(1); }
            emit_byte(OP_LD); emit_byte(rd); emit_byte(addr);
        }
        else if (!strcmp(mnemonic, "ST")) {
            int rs = parse_reg(a1);
            if (rs < 0) { fprintf(stderr, "Bad register for ST\n"); exit(1); }
            uint8_t addr;
            if (!parse_int(a2, &addr)) { fprintf(stderr, "Bad address for ST\n"); exit(1); }
            emit_byte(OP_ST); emit_byte(rs); emit_byte(addr);
        }
        else {
            fprintf(stderr, "Unknown mnemonic: '%s'\n", mnemonic);
            exit(1);
        }
    }

    fclose(f);
    resolve_patches();

    *out = malloc(code_len);
    memcpy(*out, code, code_len);
    *out_len = code_len;
}
