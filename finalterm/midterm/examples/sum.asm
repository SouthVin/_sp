; sum.asm — Sum integers from 1 to 10
; R0 = running sum, R1 = counter
    MVI R0, 0       ; sum = 0
    MVI R1, 10      ; counter = 10
LOOP:
    ADD R0, R1      ; sum += counter
    DEC R1
    JNZ LOOP
    OUT R0          ; print sum (55)
    HLT
