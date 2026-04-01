all: sum

sum: sum.c
	# Compile sum.c as a 32-bit, standalone, non-PIE executable
	# This is required by the assignment for the loader to work
	gcc -m32 -no-pie -nostdlib sum.c -o sum

clean:
	rm -f sum