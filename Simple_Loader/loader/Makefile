#Create lib_simpleloader.so from loader.c

#Provide the command for cleanup
CC := gcc
CFLAGS := -m32 -fPIC
TARGET := ../bin/libsimpleloader.so
SRC := loader.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -shared $< -o $@

clean:
	rm -f $(TARGET)