CC       = gcc
CFLAGS   = -Wall -Wextra -std=c99 -pedantic
TARGET   = toyemu
OBJS     = main.o toyemu.o asm.o

EXAMPLES = examples/hello.asm examples/fib.asm examples/multiply.asm \
           examples/countdown.asm examples/sum.asm examples/subroutine.asm

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c toyemu.h
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TARGET)
	@for f in $(EXAMPLES); do \
		echo "========== $$f =========="; \
		./$(TARGET) $$f; \
		echo ""; \
	done

clean:
	rm -f $(OBJS) $(TARGET)
