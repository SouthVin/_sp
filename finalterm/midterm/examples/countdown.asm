; countdown.asm — Count down from 10 to 1
    MVI R0, 10
LOOP:
    OUT R0          ; print current value
    DEC R0
    JNZ LOOP        ; loop while not zero
    HLT
