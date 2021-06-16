#include <stdio.h>
#include <unistd.h>
#define BUFSIZE 100


char buf[BUFSIZE];

int bufp = 0;

char c;

char kbgetch(void){
	if(bufp > 0) return buf[--bufp];  /* return a buffered character or get a new one */
	else{
		read(STDIN_FILENO, &c ,1);
		return c;
	
	}
}

void ungetch(int c){
	if(bufp	>= BUFSIZE) printf("ungetch: too many character\n"); 
	else buf[bufp++] = c;
}
