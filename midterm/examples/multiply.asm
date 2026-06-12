; multiply.asm — Multiply 7 × 9 using repeated addition
; R0 = multiplicand, R1 = multiplier, R2 = result
    MVI R0, 7
    MVI R1, 9
    MVI R2, 0
LOOP:
    CMP R1, R3      ; compare multiplier with 0 (R3 starts at 0)
    JZ DONE
    ADD R2, R0      ; result += multiplicand
    DEC R1          ; multiplier--
    JMP LOOP
DONE:
    OUT R2          ; print result (63)
    HLT
