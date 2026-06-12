; subroutine.asm — Demonstrate CALL/RET with a square function
; square(n) = n * n  (computed via repeated addition)
; Compute 5² = 25
    MVI R0, 5       ; argument n = 5
    CALL SQUARE
    OUT R0          ; print result (25)
    HLT

SQUARE:
    PUSH R1         ; save R1 (multiplicand)
    PUSH R2         ; save R2 (loop counter)
    MOV R1, R0      ; multiplicand = n
    MOV R2, R0      ; counter = n
    MVI R0, 0       ; result = 0
SQ_LOOP:
    CMP R2, R3      ; counter == 0?
    JZ SQ_DONE
    ADD R0, R1      ; result += multiplicand
    DEC R2          ; counter--
    JMP SQ_LOOP
SQ_DONE:
    POP R2
    POP R1
    RET
