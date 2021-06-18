#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>

#define STDIN STDIN_FILENO
#define STDOUT STDOUT_FILENO
#define STERR STDERR_FILENO

static struct termios save_term; /* previous term settings */
static int termsaved; /* status of function */


/**************************** RAW MODE ******************************************/
int enterraw(int fd)
{

	struct termios trm;

	if(tcgetattr(fd, &save_term) < 0) return -1; /* failed to get attr */

	trm = save_term;

	trm.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	/* echo, conical, extended input processing, signal chars set to off */

	trm.c_iflag &= ~(ICRNL | ISTRIP | IXON); /* add BRKINT for disableing sigint later */
	/* CR-toNL, input parity, output flow control, and 8th bit stripping set to off */

	trm.c_cflag &= ~(CSIZE | PARENB);
	/* clear size bits, disable parity checking */

	trm.c_cflag |= CS8;
	/* set char size 8 */

	trm.c_oflag &= ~(OPOST);
	/* disable output processing */

	trm.c_cc[VMIN] = 1; /* read on byte at a time */
	trm.c_cc[VTIME] = 0; /* no timer */

	/* apply changes */
	if(tcsetattr(fd, TCSAFLUSH, &trm) < 0) return -1;

	termsaved =1;
	
	return 0;
}

int exitraw(int fd){ /* set term to vals saved at begining of program */
	if(termsaved)
		if(tcsetattr(fd, TCSAFLUSH, &save_term) < 0) return -1;
	return 0;
}

int getwindowsize(struct winsize winstruc){
	if(ioctl(STDIN, TIOCGWINSZ, (char *)&winstruc) < 0) return -1;
	printf("winrows: %d wincols: %d\n", winstruc.ws_row, winstruc.ws_col);
	return 0;
}
















int main()
{

	enterraw(STDIN);



	

	char c;
	while(c != 'q'){
		read(STDIN, &c, 1);
		if(c == 13) printf(" this is thirteen I guess %d (%c)\n\r",c,c);

		else printf("%d (%c)\n\r",c,c);
	}


	exitraw(STDIN);
}	



