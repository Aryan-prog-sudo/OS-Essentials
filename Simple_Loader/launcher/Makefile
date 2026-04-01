#Compile the launch.c by linking it with the lib_simpleloader.so

#Provide the command for cleanup
CC := gcc
CFLAGS := -m32 -I../loader
LDFLAGS := -L../bin -lsimpleloader -Wl,-rpath,'$$ORIGIN'
TARGET := ../bin/launch
SRC := launch.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET)