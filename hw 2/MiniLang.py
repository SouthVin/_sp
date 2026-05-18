import sys

# --- LEXER DEFINITIONS ---
TOK_EOF, TOK_ID, TOK_NUM = "EOF", "ID", "NUM"
TOK_VAR, TOK_DEF, TOK_WHILE, TOK_IF, TOK_ELSE, TOK_PRINT, TOK_RETURN = "var", "def", "while", "if", "else", "print", "return"
TOK_ASSIGN, TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV = "=", "+", "-", "*", "/"
TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_SEMI = "(", ")", "{", "}", ";"
TOK_EQ, TOK_GT, TOK_LT = "==", ">", "<"

class Token:
    def __init__(self, type_, value=None):
        self.type = type_
        self.value = value
    def __repr__(self):
        return f"Token({self.type}, {self.value})"

class Lexer:
    def __init__(self, text):
        self.text = text
        self.pos = 0
    
    def error(self):
        raise SyntaxError(f"Invalid character context at position {self.pos}")

    def next_token(self):
        while self.pos < len(self.text) and self.text[self.pos].isspace():
            self.pos += 1

        if self.pos >= len(self.text):
            return Token(TOK_EOF)

        char = self.text[self.pos]

        # Single characters
        if char == '+': self.pos += 1; return Token(TOK_PLUS, '+')
        if char == '-': self.pos += 1; return Token(TOK_MINUS, '-')
        if char == '*': self.pos += 1; return Token(TOK_MUL, '*')
        if char == '/': self.pos += 1; return Token(TOK_DIV, '/')
        if char == '(': self.pos += 1; return Token(TOK_LPAREN, '(')
        if char == ')': self.pos += 1; return Token(TOK_RPAREN, ')')
        if char == '{': self.pos += 1; return Token(TOK_LBRACE, '{')
        if char == '}': self.pos += 1; return Token(TOK_RBRACE, '}')
        if char == ';': self.pos += 1; return Token(TOK_SEMI, ';')

        # Relational or Assignment Operators
        if char == '=':
            self.pos += 1
            if self.pos < len(self.text) and self.text[self.pos] == '=':
                self.pos += 1
                return Token(TOK_EQ, '==')
            return Token(TOK_ASSIGN, '=')
        if char == '>': self.pos += 1; return Token(TOK_GT, '>')
        if char == '<': self.pos += 1; return Token(TOK_LT, '<')

        # Numeric Literals
        if char.isdigit():
            val = ""
            while self.pos < len(self.text) and self.text[self.pos].isdigit():
                val += self.text[self.pos]
                self.pos += 1
            return Token(TOK_NUM, int(val))

        # Keywords or Identifiers
        if char.isalpha() or char == '_':
            val = ""
            while self.pos < len(self.text) and (self.text[self.pos].isalnum() or self.text[self.pos] == '_'):
                val += self.text[self.pos]
                self.pos += 1
            
            keywords = {
                "var": TOK_VAR, "def": TOK_DEF, "while": TOK_WHILE, 
                "if": TOK_IF, "else": TOK_ELSE, "print": TOK_PRINT, "return": TOK_RETURN
            }
            if val in keywords:
                return Token(keywords[val], val)
            return Token(TOK_ID, val)

        self.error()

# --- COMPILER & EMITTER ---
class Compiler:
    def __init__(self, lexer):
        self.lexer = lexer
        self.cur_token = self.lexer.next_token()
        self.bytecode = []
        self.globals = set()
        self.locals = {}      # Format: {name: offset_index}
        self.functions = {}   # Format: {name: bytecode_address}
        self.label_counter = 0

    def error(self, msg):
        raise RuntimeError(f"Compiler Error: {msg} at token {self.cur_token}")

    def consume(self, token_type):
        if self.cur_token.type == token_type:
            self.cur_token = self.lexer.next_token()
        else:
            self.error(f"Expected token type {token_type}")

    def emit(self, instr, arg=None):
        self.bytecode.append((instr, arg))
        return len(self.bytecode) - 1

    def patch_label(self, instr_idx, target_address):
        instr, _ = self.bytecode[instr_idx]
        self.bytecode[instr_idx] = (instr, target_address)

    def compile(self):
        # Entry vector routing jump to main() function routine execution block
        main_jump_idx = self.emit("JMP", None)
        
        while self.cur_token.type != TOK_EOF:
            if self.cur_token.type == TOK_VAR:
                self.global_decl()
            elif self.cur_token.type == TOK_DEF:
                self.func_decl()
            else:
                self.error("Global space permits only function or variable signatures")

        if "main" not in self.functions:
            self.error("Missing declaration boundary root entrance definition 'main()'")
        
        # Patch structural entry point to jump directly to main loop footprint sequence
        self.patch_label(main_jump_idx, self.functions["main"])
        return self.bytecode, self.globals

    def global_decl(self):
        self.consume(TOK_VAR)
        name = self.cur_token.value
        self.consume(TOK_ID)
        self.consume(TOK_SEMI)
        self.globals.add(name)

    def func_decl(self):
        self.consume(TOK_DEF)
        func_name = self.cur_token.value
        self.consume(TOK_ID)
        self.functions[func_name] = len(self.bytecode)
        
        self.consume(TOK_LPAREN)
        self.consume(TOK_RPAREN)
        self.consume(TOK_LBRACE)

        self.locals = {} # Clean scope allocation tracking map bindings
        
        # Parse internal variable frames
        while self.cur_token.type == TOK_VAR:
            self.consume(TOK_VAR)
            var_name = self.cur_token.value
            self.consume(TOK_ID)
            self.consume(TOK_SEMI)
            if var_name in self.locals:
                self.error(f"Redeclared local instance field '{var_name}'")
            self.locals[var_name] = len(self.locals)
            self.emit("ALLOC", 1)

        # Parse active functional processing statements
        while self.cur_token.type != TOK_RBRACE and self.cur_token.type != TOK_EOF:
            self.stmt()

        # Structural safe exit fallback sequence
        self.emit("PUSH", 0)
        self.emit("RET")
        self.consume(TOK_RBRACE)

    def stmt(self):
        if self.cur_token.type == TOK_PRINT:
            self.consume(TOK_PRINT)
            self.expr()
            self.consume(TOK_SEMI)
            self.emit("PRINT")
        elif self.cur_token.type == TOK_RETURN:
            self.consume(TOK_RETURN)
            self.expr()
            self.consume(TOK_SEMI)
            self.emit("RET")
        elif self.cur_token.type == TOK_WHILE:
            self.while_stmt()
        elif self.cur_token.type == TOK_IF:
            self.if_stmt()
        elif self.cur_token.type == TOK_ID:
            self.assign_stmt()
        elif self.cur_token.type == TOK_LBRACE:
            self.consume(TOK_LBRACE)
            while self.cur_token.type != TOK_RBRACE:
                self.stmt()
            self.consume(TOK_RBRACE)
        else:
            self.error(f"Unexpected token start pattern logic variant mapping: {self.cur_token.value}")

    def assign_stmt(self):
        name = self.cur_token.value
        self.consume(TOK_ID)
        self.consume(TOK_ASSIGN)
        self.expr()
        self.consume(TOK_SEMI)

        if name in self.locals:
            self.emit("STORE_LOCAL", self.locals[name])
        elif name in self.globals:
            self.emit("STORE_GLOBAL", name)
        else:
            self.error(f"Identifier allocation target lookup fault: Variable '{name}' is not defined.")

    def while_stmt(self):
        self.consume(TOK_WHILE)
        loop_start = len(self.bytecode)
        self.consume(TOK_LPAREN)
        self.expr()
        self.consume(TOK_RPAREN)

        jz_idx = self.emit("JZ", None)
        self.stmt()
        self.emit("JMP", loop_start)
        
        loop_end = len(self.bytecode)
        self.patch_label(jz_idx, loop_end)

    def if_stmt(self):
        self.consume(TOK_IF)
        self.consume(TOK_LPAREN)
        self.expr()
        self.consume(TOK_RPAREN)
        
        jz_idx = self.emit("JZ", None)
        self.stmt()
        
        if self.cur_token.type == TOK_ELSE:
            self.consume(TOK_ELSE)
            jmp_idx = self.emit("JMP", None)
            
            else_start = len(self.bytecode)
            self.patch_label(jz_idx, else_start)
            
            self.stmt()
            self.patch_label(jmp_idx, len(self.bytecode))
        else:
            self.patch_label(jz_idx, len(self.bytecode))

    def expr(self):
        self.term()
        while self.cur_token.type in (TOK_PLUS, TOK_MINUS, TOK_EQ, TOK_GT, TOK_LT):
            op = self.cur_token.type
            self.consume(op)
            self.term()
            if op == TOK_PLUS: self.emit("ADD")
            elif op == TOK_MINUS: self.emit("SUB")
            elif op == TOK_EQ: self.emit("CMPEQ")
            elif op == TOK_GT: self.emit("CMPGT")
            elif op == TOK_LT: self.emit("CMPLT")

    def term(self):
        self.factor()
        while self.cur_token.type in (TOK_MUL, TOK_DIV):
            op = self.cur_token.type
            self.consume(op)
            self.factor()
            if op == TOK_MUL: self.emit("MUL")
            elif op == TOK_DIV: self.emit("DIV")

    def factor(self):
        if self.cur_token.type == TOK_NUM:
            self.emit("PUSH", self.cur_token.value)
            self.consume(TOK_NUM)
        elif self.cur_token.type == TOK_ID:
            name = self.cur_token.value
            self.consume(TOK_ID)
            
            if self.cur_token.type == TOK_LPAREN: # Call branch sequence check
                self.consume(TOK_LPAREN)
                self.consume(TOK_RPAREN)
                if name not in self.functions:
                    self.error(f"Functional reference lookup mismatch targeting object identifier '{name}'")
                self.emit("CALL", name)
            else: # Variable loading routine
                if name in self.locals:
                    self.emit("LOAD_LOCAL", self.locals[name])
                elif name in self.globals:
                    self.emit("LOAD_GLOBAL", name)
                else:
                    self.error(f"Undeclared literal variable entity mapping object pointer assignment tracking reference: {name}")
        elif self.cur_token.type == TOK_LPAREN:
            self.consume(TOK_LPAREN)
            self.expr()
            self.consume(TOK_RPAREN)
        else:
            self.error("Malformed expression segment structural failure syntax rule layout exception")

# --- VIRTUAL MACHINE EXECUTION PIPELINE ---
class VirtualMachine:
    def __init__(self, bytecode, globals_set, functions_map):
        self.bytecode = bytecode
        self.globals = {name: 0 for name in globals_set}
        self.functions = functions_map
        self.data_stack = []
        self.call_stack = [] # Format elements stack context records: (return_ip, locals_array)
        self.locals = []
        self.ip = 0

    def run(self):
        while self.ip < len(self.bytecode):
            instr, arg = self.bytecode[self.ip]
            
            if instr == "PUSH":
                self.data_stack.append(arg)
            elif instr == "ALLOC":
                for _ in range(arg):
                    self.locals.append(0)
            elif instr == "LOAD_LOCAL":
                self.data_stack.append(self.locals[arg])
            elif instr == "STORE_LOCAL":
                self.locals[arg] = self.data_stack.pop()
            elif instr == "LOAD_GLOBAL":
                self.data_stack.append(self.globals[arg])
            elif instr == "STORE_GLOBAL":
                self.globals[arg] = self.data_stack.pop()
            elif instr == "ADD":
                b = self.data_stack.pop()
                a = self.data_stack.pop()
                self.data_stack.append(a + b)
            elif instr == "SUB":
                b = self.data_stack.pop()
                a = self.data_stack.pop()
                self.data_stack.append(a - b)
            elif instr == "MUL":
                b = self.data_stack.pop()
                a = self.data_stack.pop()
                self.data_stack.append(a * b)
            elif instr == "DIV":
                b = self.data_stack.pop()
                a = self.data_stack.pop()
                self.data_stack.append(int(a / b))
            elif instr == "CMPEQ":
                self.data_stack.append(1 if self.data_stack.pop() == self.data_stack.pop() else 0)
            elif instr == "CMPGT":
                b = self.data_stack.pop()
                a = self.data_stack.pop()
                self.data_stack.append(1 if a > b else 0)
            elif instr == "CMPLT":
                b = self.data_stack.pop()
                a = self.data_stack.pop()
                self.data_stack.append(1 if a < b else 0)
            elif instr == "JMP":
                self.ip = arg
                continue
            elif instr == "JZ":
                condition = self.data_stack.pop()
                if condition == 0:
                    self.ip = arg
                    continue
            elif instr == "CALL":
                self.call_stack.append((self.ip + 1, self.locals))
                self.locals = [] # Clear/instantiate independent frame stack boundaries
                self.ip = self.functions[arg]
                continue
            elif instr == "RET":
                ret_val = self.data_stack.pop()
                if not self.call_stack: # Terminate execution frame pipeline if root returns
                    break
                self.ip, self.locals = self.call_stack.pop()
                self.data_stack.append(ret_val)
                continue
            elif instr == "PRINT":
                print(f"[VM Runtime Print Event output]: {self.data_stack.pop()}")
            
            self.ip += 1

# --- SYSTEM INTEGRATION PIPELINE VERIFIER ---
if __name__ == "__main__":
    mini_lang_code = """
    var global_counter;

    def calculate_factorial() {
        var n;
        var result;
        n = 5;
        result = 1;

        while (n > 0) {
            result = result * n;
            n = n - 1;
        }
        
        return result;
    }

    def main() {
        var computation;
        global_counter = 100;
        print global_counter;
        
        computation = calculate_factorial();
        print computation;
    }
    """

    print("=== STARTING COMPILATION PIPELINE ===")
    lexer = Lexer(mini_lang_code)
    compiler = Compiler(lexer)
    bytecode, globals_set = compiler.compile()
    
    print("\n=== GENERATED BYTECODE INTERMEDIATE REPRESENTATION ===")
    for idx, (instr, arg) in enumerate(bytecode):
        print(f"{idx:04d}: {instr:<15} {str(arg) if arg is not None else ''}")

    print("\n=== NATIVE EXECUTING VM PIPELINE APPLICATION ===")
    vm = VirtualMachine(bytecode, globals_set, compiler.functions)
    vm.run()