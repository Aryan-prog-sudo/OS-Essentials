LIB_NAME = libsmartloader.so

all: ../bin/$(LIB_NAME)

../bin/$(LIB_NAME): loader.c loader.h
	# Create the bin directory if it doesn't exist
	@mkdir -p ../bin
	# Compile loader.c as a 32-bit shared library
	gcc -m32 -fPIC -shared loader.c -o ../bin/$(LIB_NAME)

clean:
	rm -f ../bin/$(LIB_NAME)