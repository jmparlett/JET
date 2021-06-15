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


/*********************************** MOVEMENT ***************************************/
int setcursorpos(int x, int y){
	char buf[32]; 
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH",y,x);
	write(STDOUT, buf, sizeof(buf));
	return 0;
}



/********************************** Editor Structure *****************************/
typedef struct frow{ /* file row struct */

	int idx; /* line num in file */
	
	struct frow *nx; /* next node */

	struct frow *pr; /* prev node */

	char *s; /* string of line */

} frow;

typedef struct editor{

	int cx, cy; /* position */
	int rowc;   /*num rows */
	int colc;   /*num cols */
	frow *fline;
	frow *cline;
} editor;

static editor J;
int initeditor(){
	setcursorpos(0,0);

	static struct winsize window; /* contains window rows and cols */
	if(ioctl(STDIN, TIOCGWINSZ, (char *)&window) < 0) return -1;	

	/* init editor vals */
	J.cx = J.cy = 0;
	J.rowc = window.ws_row;
	J.colc = window.ws_col;
	J.fline = NULL;
	J.cline = NULL;

	
	
	
	return 0;
}



typedef struct scrbuf{

	int len;

	char *c;

} scrbuf; 

void scrbufappend(scrbuf *buf, char *s, unsigned len){
	
	if((buf->c = (char *)realloc(buf->c, buf->len+len)) == NULL) printf("error: scrbufappend() failed to alloc memory");

	memcpy(buf->c+buf->len, s, len);
	buf->len = buf->len+len;
	//printf("%s %d\n", buf->c, buf->len);

	return;
	
}
void freescrbuf(scrbuf buf){
	free(buf.c);
}

int drawscreen(){
	scrbuf buf = {0, NULL};

	write(STDOUT, "\x1b[2J", 4); /* clear screen */
	write(STDOUT, "\x1b[H", 3); /* move cursor home */
	
	int i=0;

	frow *t = J.cline; /* file row pointer */

	for(; i < J.rowc; ++i){
		if(t != NULL){
			
			scrbufappend(&buf, t->s, strlen(t->s));
			scrbufappend(&buf, "\r\n", 3);

			t=t->nx;
		} else {
			scrbufappend(&buf, "~\r\n", 3);
		}
	//	write(STDOUT, "~~\r\n", 4);
	}
	//printf("%s",buf.c);
	write(STDOUT, buf.c, buf.len);
	freescrbuf(buf);

	//setcursorpos(J.cx, J.cy);
	return 0;



}


/************************File Buffer******************************/


void addrow(void){
	if(J.fline == NULL){ /* no lines yet */
		frow *p = (frow *)malloc(sizeof(frow));	
		p-> idx = 1;
		p->nx = NULL;
		p->pr = NULL;
		p->s = (char *)malloc(J.colc);
		J.fline = J.cline = p;
		
	} else { /* add new row after J.cline */
		
		frow *new = (frow *)malloc(sizeof(frow));
		new->idx = J.cline->idx+1;
		new->nx = J.cline->nx;
		new->pr = J.cline;
		new->s = (char *)malloc(J.colc);
		J.cline->nx = new;
		if(new->nx != NULL){
			new->nx->pr = new;
			frow *t=new;
			while((t=t->nx) != NULL) t->idx++; /* increment line num of all lines aJ.fline of J.cline */
		}
	}
	return;

}















int main()
{

	enterraw(STDIN);
	initeditor();
	drawscreen();



	
	int i=5;
	while(i--) addrow();
	printf("done adding\n");
	while((J.cline=J.cline->nx) != NULL) strcpy(J.cline->s, "Hello, World\0");
	J.cline = J.fline->nx;

	char c;
	while(c != 'q'){
		drawscreen();
		read(STDIN, &c, 1);
	}


	drawscreen();
	exitraw(STDIN);
}	



