CC := gcc
CFLAGS := -m32 -no-pie -nostdlib
TARGET := fib
SRC := fib.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET)