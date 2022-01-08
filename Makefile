jet: jet.c
	$(CC) jet.c -o jet -Wall -Wextra -pedantic -std=c99

install:
	make jet 
	mv jet /usr/local/bin/
