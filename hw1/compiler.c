#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 64
#define MAX_SYMBOLS 256
#define SRC_SIZE 4096

// --- LEXER DEFINITIONS ---
typedef enum {
    TOKEN_EOF, TOKEN_ID, TOKEN_NUM,
    TOKEN_WHILE, TOKEN_INT, TOKEN_RETURN,
    TOKEN_ASSIGN, TOKEN_PLUS, TOKEN_MINUS, TOKEN_MUL, TOKEN_DIV,
    TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE,
    TOKEN_COMMA, TOKEN_SEMI, TOKEN_EQ
} TokenType;

typedef struct {
    TokenType type;
    char text[MAX_TOKEN_LEN];
    int value;
} Token;

// --- SYMBOL TABLE SCOPES ---
typedef struct {
    char name[MAX_TOKEN_LEN];
    int offset;    // Offset relative to Base Pointer (BP) for locals, or global addr
    int is_local;  // 1 if local variable, 0 if global variable
} Symbol;

// Global Compiler State
char source[SRC_SIZE];
int src_pos = 0;
Token cur_token;
Symbol symbol_table[MAX_SYMBOLS];
int symbol_count = 0;

int label_counter = 0;
int local_offset_counter = 0; // Track frame offset for local variables
int is_inside_function = 0;

// --- PROTOTYPES ---
void next_token();
void error(const char *msg);
void skip(TokenType type, const char *err_msg);
void program();
void function_decl();
void stmt();
void while_stmt();
void assign_stmt();
void return_stmt();
void expr();
void term();
void factor();

// --- LEXER IMPLEMENTATION ---
void next_token() {
    while (source[src_pos] && isspace(source[src_pos])) {
        src_pos++;
    }

    if (!source[src_pos]) {
        cur_token.type = TOKEN_EOF;
        strcpy(cur_token.text, "EOF");
        return;
    }

    char c = source[src_pos];

    // Single character punctuation symbols
    if (c == '+') { cur_token.type = TOKEN_PLUS; strcpy(cur_token.text, "+"); src_pos++; return; }
    if (c == '-') { cur_token.type = TOKEN_MINUS; strcpy(cur_token.text, "-"); src_pos++; return; }
    if (c == '*') { cur_token.type = TOKEN_MUL; strcpy(cur_token.text, "*"); src_pos++; return; }
    if (c == '/') { cur_token.type = TOKEN_DIV; strcpy(cur_token.text, "/"); src_pos++; return; }
    if (c == '(') { cur_token.type = TOKEN_LPAREN; strcpy(cur_token.text, "("); src_pos++; return; }
    if (c == ')') { cur_token.type = TOKEN_RPAREN; strcpy(cur_token.text, ")"); src_pos++; return; }
    if (c == '{') { cur_token.type = TOKEN_LBRACE; strcpy(cur_token.text, "{"); src_pos++; return; }
    if (c == '}') { cur_token.type = TOKEN_RBRACE; strcpy(cur_token.text, "}"); src_pos++; return; }
    if (c == ',') { cur_token.type = TOKEN_COMMA; strcpy(cur_token.text, ","); src_pos++; return; }
    if (c == ';') { cur_token.type = TOKEN_SEMI; strcpy(cur_token.text, ";"); src_pos++; return; }

    // Multi-char operators/comparisons
    if (c == '=') {
        src_pos++;
        if (source[src_pos] == '=') {
            cur_token.type = TOKEN_EQ;
            strcpy(cur_token.text, "==");
            src_pos++;
        } else {
            cur_token.type = TOKEN_ASSIGN;
            strcpy(cur_token.text, "=");
        }
        return;
    }

    // Number literals
    if (isdigit(c)) {
        int i = 0;
        while (source[src_pos] && isdigit(source[src_pos])) {
            cur_token.text[i++] = source[src_pos++];
        }
        cur_token.text[i] = '\0';
        cur_token.type = TOKEN_NUM;
        cur_token.value = atoi(cur_token.text);
        return;
    }

    // Identifiers & Keywords
    if (isalpha(c) || c == '_') {
        int i = 0;
        while (source[src_pos] && (isalnum(source[src_pos]) || source[src_pos] == '_')) {
            cur_token.text[i++] = source[src_pos++];
        }
        cur_token.text[i] = '\0';

        if (strcmp(cur_token.text, "while") == 0) cur_token.type = TOKEN_WHILE;
        else if (strcmp(cur_token.text, "int") == 0) cur_token.type = TOKEN_INT;
        else if (strcmp(cur_token.text, "return") == 0) cur_token.type = TOKEN_RETURN;
        else cur_token.type = TOKEN_ID;
        return;
    }

    error("Unknown token encountered");
}

// --- UTILITY CODE HELPERS ---
void error(const char *msg) {
    fprintf(stderr, "Compiler Error: %s at token '%s'\n", msg, cur_token.text);
    exit(1);
}

void skip(TokenType type, const char *err_msg) {
    if (cur_token.type == type) {
        next_token();
    } else {
        error(err_msg);
    }
}

int next_label() {
    return label_counter++;
}

// --- SYMBOL TABLE OPERATIONS ---
void add_symbol(const char *name, int is_local) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0 && symbol_table[i].is_local == is_local) {
            error("Redeclaration of variable");
        }
    }
    strcpy(symbol_table[symbol_count].name, name);
    symbol_table[symbol_count].is_local = is_local;
    
    if (is_local) {
        local_offset_counter += 4; // Allocate 4 bytes on stack frame
        symbol_table[symbol_count].offset = local_offset_counter;
    } else {
        symbol_table[symbol_count].offset = 0; // Simplification for globals
    }
    symbol_count++;
}

int lookup_symbol(const char *name, Symbol *out_sym) {
    // 1. Always look up Local Scope first to support scoping mechanics
    for (int i = symbol_count - 1; i >= 0; i--) {
        if (strcmp(symbol_table[i].name, name) == 0 && symbol_table[i].is_local == 1) {
            *out_sym = symbol_table[i];
            return 1; // Found in local scope
        }
    }
    // 2. Fall back to Global Scope
    for (int i = symbol_count - 1; i >= 0; i--) {
        if (strcmp(symbol_table[i].name, name) == 0 && symbol_table[i].is_local == 0) {
            *out_sym = symbol_table[i];
            return 2; // Found in global scope
        }
    }
    return 0; // Not found
}

void clear_local_symbols() {
    int i = 0;
    while (i < symbol_count) {
        if (symbol_table[i].is_local) {
            // Remove local variable to avoid namespace leaking outside the function
            for (int j = i; j < symbol_count - 1; j++) {
                symbol_table[j] = symbol_table[j + 1];
            }
            symbol_count--;
        } else {
            i++;
        }
    }
    local_offset_counter = 0;
}

// --- PARSER / CODE GENERATOR IMPLEMENTATION ---

void program() {
    while (cur_token.type != TOKEN_EOF) {
        if (cur_token.type == TOKEN_INT) {
            next_token();
            char name[MAX_TOKEN_LEN];
            strcpy(name, cur_token.text);
            skip(TOKEN_ID, "Expected variable or function name identifier");
            
            if (cur_token.type == TOKEN_LPAREN) {
                // It's a function declaration
                is_inside_function = 1;
                printf("\n_func_%s:\n", name);
                function_decl();
                is_inside_function = 0;
            } else {
                // It's a global variable declaration
                add_symbol(name, 0);
                printf("\t.global %s\n", name);
                skip(TOKEN_SEMI, "Expected ';' after global declaration");
            }
        } else {
            error("Global items must begin with type identifier 'int'");
        }
    }
}

void function_decl() {
    skip(TOKEN_LPAREN, "Expected '('");
    // Parse arguments if any (simplified here)
    skip(TOKEN_RPAREN, "Expected ')'");
    skip(TOKEN_LBRACE, "Expected function body opening '{'");
    
    // Function Prologue
    printf("\tPUSH BP\n");
    printf("\tMOV BP, SP\n");
    // Space placeholder for local variables context will allocate dynamically below
    
    while (cur_token.type != TOKEN_RBRACE && cur_token.type != TOKEN_EOF) {
        if (cur_token.type == TOKEN_INT) {
            next_token();
            add_symbol(cur_token.text, 1);
            printf("\tSUB SP, 4\t; Allocate space for local variable '%s'\n", cur_token.text);
            skip(TOKEN_ID, "Expected identifier");
            skip(TOKEN_SEMI, "Expected ';'");
        } else {
            stmt();
        }
    }
    
    // Function Epilogue (Fallback structural end if return statement is absent)
    printf("\tMOV SP, BP\n");
    printf("\tPOP BP\n");
    printf("\tRET\n");
    
    skip(TOKEN_RBRACE, "Expected closing '}'");
    clear_local_symbols(); // Clear local assignments out of context scope
}

void stmt() {
    if (cur_token.type == TOKEN_WHILE) {
        while_stmt();
    } else if (cur_token.type == TOKEN_ID) {
        assign_stmt();
    } else if (cur_token.type == TOKEN_RETURN) {
        return_stmt();
    } else if (cur_token.type == TOKEN_LBRACE) {
        next_token();
        while (cur_token.type != TOKEN_RBRACE && cur_token.type != TOKEN_EOF) {
            stmt();
        }
        skip(TOKEN_RBRACE, "Expected closing '}'");
    } else {
        error("Unexpected statement start syntax structural rule");
    }
}

// Homework Objective 1: The 'while' Loop Syntax Implementation
void while_stmt() {
    int start_lbl = next_label();
    int end_lbl = next_label();

    skip(TOKEN_WHILE, "Expected 'while'");
    skip(TOKEN_LPAREN, "Expected condition opening '('");

    printf("L%d:\t\t\t; Loop Condition Start Point\n", start_lbl);
    expr(); // Generates expression code mapping onto runtime valuation stack
    
    skip(TOKEN_RPAREN, "Expected condition closing ')'");

    printf("\tJZ L%d\t\t; Jump to End if false Evaluation\n", end_lbl);
    
    stmt(); // Evaluates structural loop execution statements body

    printf("\tJMP L%d\t\t; Re-evaluate condition loop branch\n", start_lbl);
    printf("L%d:\t\t\t; Loop Exit Endpoint Break\n", end_lbl);
}

void assign_stmt() {
    char target_name[MAX_TOKEN_LEN];
    strcpy(target_name, cur_token.text);
    
    Symbol sym;
    int scope = lookup_symbol(target_name, &sym);
    if (scope == 0) {
        error("Undeclared variable reference assignment mutation targeting non-existent context object");
    }

    skip(TOKEN_ID, "Expected variable token name");
    skip(TOKEN_ASSIGN, "Expected '=' assignment mapping operator symbol");
    
    expr(); 
    
    skip(TOKEN_SEMI, "Expected termination symbol ';'");

    // Homework Objective 3: Checking Local vs Global Offset lookup handling
    if (sym.is_local) {
        printf("\tSTORE [BP-%d]\t; Assign to local var '%s'\n", sym.offset, target_name);
    } else {
        printf("\tSTORE [%s]\t; Assign to global var '%s'\n", sym.name, target_name);
    }
}

void return_stmt() {
    skip(TOKEN_RETURN, "Expected 'return'");
    expr();
    skip(TOKEN_SEMI, "Expected ';'");
    
    // Function Return Epilogue sequence
    printf("\tPOP AX\t\t; Return value placed in Accumulator register\n");
    printf("\tMOV SP, BP\n");
    printf("\tPOP BP\n");
    printf("\tRET\n");
}

void expr() {
    term();
    while (cur_token.type == TOKEN_PLUS || cur_token.type == TOKEN_MINUS || cur_token.type == TOKEN_EQ) {
        TokenType op = cur_token.type;
        next_token();
        term();
        if (op == TOKEN_PLUS)  printf("\tADD\n");
        if (op == TOKEN_MINUS) printf("\tSUB\n");
        if (op == TOKEN_EQ)    printf("\tCMPEQ\n");
    }
}

void term() {
    factor();
    while (cur_token.type == TOKEN_MUL || cur_token.type == TOKEN_DIV) {
        TokenType op = cur_token.type;
        next_token();
        factor();
        if (op == TOKEN_MUL) printf("\tMUL\n");
        if (op == TOKEN_DIV) printf("\tDIV\n");
    }
}

void factor() {
    if (cur_token.type == TOKEN_NUM) {
        printf("\tPUSH %d\n", cur_token.value);
        next_token();
    } else if (cur_token.type == TOKEN_ID) {
        char name[MAX_TOKEN_LEN];
        strcpy(name, cur_token.text);
        next_token();
        
        if (cur_token.type == TOKEN_LPAREN) {
            // Homework Objective 2: Function Calling Mechanism Execution
            skip(TOKEN_LPAREN, "Expected argument list entry opening character group");
            int arg_count = 0;
            if (cur_token.type != TOKEN_RPAREN) {
                expr();
                arg_count++;
                while (cur_token.type == TOKEN_COMMA) {
                    next_token();
                    expr();
                    arg_count++;
                }
            }
            skip(TOKEN_RPAREN, "Expected closing statement structure ')'");
            printf("\tCALL _func_%s\n", name);
            if (arg_count > 0) {
                printf("\tADD SP, %d\t; Clean arguments off the stack frame\n", arg_count * 4);
            }
            printf("\tPUSH AX\t\t; Push returned value result onto tracking context heap\n");
        } else {
            // Factor is a simple variable identifier reference look-up
            Symbol sym;
            int scope = lookup_symbol(name, &sym);
            if (scope == 0) {
                error("Variable runtime tracking mapping not found context error allocation missing reference lookup exception");
            }
            if (sym.is_local) {
                printf("\tLOAD [BP-%d]\t; Load local var '%s'\n", sym.offset, name);
            } else {
                printf("\tLOAD [%s]\t; Load global var '%s'\n", sym.name, name);
            }
        }
    } else if (cur_token.type == TOKEN_LPAREN) {
        next_token();
        expr();
        skip(TOKEN_RPAREN, "Expected nested block mathematical execution boundary grouping token wrapper alignment close");
    } else {
        error("Malformed evaluation expression expression logic error encountered execution");
    }
}

// --- TESTING PIPELINE DRIVER ---
int main() {
    // A sample test string written in the language compiled by our p0 baseline code specification
    const char *test_program = 
        "int globalVar;\n"
        "int main() {\n"
        "    int x;\n"
        "    x = 5;\n"
        "    while (x == 5) {\n"
        "        x = x - 1;\n"
        "    }\n"
        "    return x;\n"
        "}\n";

    strcpy(source, test_program);
    src_pos = 0;

    printf("; --- COMPILING SAMPLE P0 PROGRAM ---\n");
    next_token(); // Initialize first compiler execution pipeline loop entry point
    program();    // Invoke root compilation tracking process
    
    return 0;
}