# Name of our loader library
LIB_NAME = smartloader
# Path to the library in the bin/ folder
LIB_DIR = ../bin
# Path to the header in the loader/ folder
INCLUDE_DIR = ../loader

all: ../bin/launch

../bin/launch: launcher.c
	@mkdir -p ../bin
	# Compile launcher.c, telling gcc where to find the
	# loader.h header (-I) and the libsmartloader.so library (-L and -l)
	# We also link against 'c' for fprintf, etc.
	gcc -m32 launcher.c -o ../bin/launch -I$(INCLUDE_DIR) -L$(LIB_DIR) -l$(LIB_NAME)

clean:
	rm -f ../bin/launch