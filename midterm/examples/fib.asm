; fib.asm — Compute Fibonacci(10) = 55
; R0 = loop counter, R1 = prev, R2 = current, R3 = temp
    MVI R0, 10      ; 9 loop iterations -> F(10)
    MVI R1, 0       ; F(0) = 0
    MVI R2, 1       ; F(1) = 1
LOOP:
    DEC R0
    JZ DONE
    MOV R3, R2      ; temp = current
    ADD R2, R1      ; current = current + prev
    MOV R1, R3      ; prev = temp
    JMP LOOP
DONE:
    OUT R2          ; print result (55)
    HLT
