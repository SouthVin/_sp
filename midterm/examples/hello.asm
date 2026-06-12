; hello.asm — Print "Hi" to console using character output
    MVI R0, 'H'
    OUTC R0
    MVI R0, 'i'
    OUTC R0
    MVI R0, '\n'
    OUTC R0
    HLT
