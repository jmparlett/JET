#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include "kbgetch.h"
#include "jet.h"

#define STDIN STDIN_FILENO
#define STDOUT STDOUT_FILENO
#define STERR STDERR_FILENO
#define SCRDIMBYTES 100 /* used in kbinput() to capture cursor x and y as a string. Must be able to capture the max screen dimensions */
static struct termios save_term; /* previous term settings */
static int termsaved; /* status of function */
char cpos[SCRDIMBYTES];  /* string used for cursor position */

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





/********************************** Editor Structure *****************************/
typedef struct frow{ /* file row struct */

	int idx; /* line num in file */
	
	int len; /* length of line */
	
	struct frow *nx; /* next node */

	struct frow *pr; /* prev node */

	char *s; /* string of line */

} frow;

typedef struct editor{

	int cx, cy; /* position */
	int rowc;   /*num rows */
	int colc;   /*num cols */
	int drawstart; /* position to begin drawing each line. Used for truncation */
	frow *fline; /*first line/ head of linked list */
	frow *cline; /* current line */
	frow *tsl; /* top screen line */
	
} editor;

static editor J;
int initeditor(){

	write(STDOUT, "\x1b[H", 3); /* send cursor to 0,0 (home) */
	
	static struct winsize window; /* contains window rows and cols */

	if(ioctl(STDIN, TIOCGWINSZ, (char *)&window) < 0) return -1;	

	/* init editor vals */
	J.cx = J.cy = 0;
	J.rowc = window.ws_row;
	J.colc = window.ws_col;
	J.fline = NULL;
	J.cline = NULL;
	J.tsl = NULL;
	J.drawstart =0;

	
	
	
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
	//write(STDOUT, "\x1b[s", 3); /* save cursor position */
	write(STDOUT, "\x1b[H", 3); /* move cursor home */
	
	int i=0;

	frow *t = J.tsl; /* file row pointer */

	for(; i < J.rowc; ++i){
		if(t != NULL){
			if(J.cx > J.colc && t == J.cline){

				int off = (J.cx / J.colc) * J.colc;

				scrbufappend(&buf, t->s+off, (t->len-off < J.colc ? t->len-off : J.colc));
				scrbufappend(&buf, "\r\n", 2);

			} else {
			/* if length of line less than columns than draw whole line else only draw chars up to last col */	
			scrbufappend(&buf, t->s, (t->len < J.colc ? t->len : J.colc));
			scrbufappend(&buf, "\r\n", 2);
			}
			t=t->nx;
		} else {
			scrbufappend(&buf, "~\r\n", 3);
		}
	}
	write(STDOUT, buf.c, buf.len);
	freescrbuf(buf);

	 /* draw cursor position wrapping x around screen */
	snprintf(cpos, SCRDIMBYTES, "\x1b[%d;%dH", J.cy, ((J.cx+1)%J.colc));
	write(STDOUT, cpos, strlen(cpos));

	return 0;



}

/*********************************** KEYBOARD INPUT ***************************************/
char kbinput(){
	char b = kbgetch();
	
	if(b == '\x1b'){ /* we have an escape sequence */
		if((b=kbgetch()) != '['){ /* it was a single escape we want to ungetch and call kbinput to handle */
			ungetch(b);
			kbinput();
		}
		else{ /* we have an escape sequence to handle */
			b = kbgetch();
			switch(b){
				case 'A': /* move up */
				if(J.cline->pr != NULL){
					if(J.cline->len > J.cline->pr->len) J.cx = J.cline->pr->len-1; /* snap to end of line if current is longer */
					J.cline = J.cline->pr;
					if(J.cy == 0) J.tsl = J.tsl->pr; /* scroll window up one line */
					else J.cy--; /* else move screen row index */
									
				}
				break;
				case 'B': /* move down */
				if(J.cline->nx != NULL){
					if(J.cline->len > J.cline->nx->len) J.cx = J.cline->nx->len-1; /* snap to end of line if current is longer */
					J.cline = J.cline->nx;
					if(J.cy == J.rowc-1) J.tsl = J.tsl->nx; /* scroll window down one line */
					else J.cy++;
				}
				break;
				case 'C': /* move right */
				if(J.cx == J.cline->len -1 && J.cline->nx != NULL){ /* move to next line */
					J.cline = J.cline->nx;
					if(J.cy == J.rowc-1) J.tsl = J.tsl->nx; /* scroll window down one line */
					else J.cy++;
	
					J.cx = 0; /* first index */
					fprintf(stdout, "J.cx: %d\n",J.cx); 
				} else if(J.cx < J.cline->len-1) {
					fprintf(stdout, "J.cx: %d\n",J.cx); 
					J.cx++;
				}
				break;
				case 'D': /* move left */
				if(J.cx == 0 && J.cline->pr != NULL){ /* move to prev line */
					J.cline = J.cline->pr;
					if(J.cy == 0) J.tsl = J.tsl->pr; /* scroll window up one line */
					else J.cy--; /* else move screen row index */
					J.cx = J.cline->len-1; /* last index */

				} else if(J.cx > 0){
				 J.cx--;
				}
				break;
			
				case 'F': /* end */
					J.cx = J.cline->len-1;	
				break;

				case 'H': /* home */
					J.cx = 0;	
				break;
				
				

			}
		}
	} else if(b == QUIT){
		 exitraw(STDIN);
		 exit(0);
	}


}
/************************File Buffer******************************/


void addrow(void){
	if(J.fline == NULL){ /* no lines yet */
		frow *p = (frow *)malloc(sizeof(frow));	
		p-> idx = 0;
		p->nx = NULL;
		p->pr = NULL;
		p->len = 0;
		p->s = (char *)malloc(J.colc);
		J.fline = J.cline = J.tsl = p;
		
	} else { /* add new row after J.cline */
		
		frow *new = (frow *)malloc(sizeof(frow));
		new->idx = J.cline->idx+1;
		new->nx = J.cline->nx;
		new->pr = J.cline;
		new->s = (char *)malloc(J.colc);
		new->len = 0;
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



	
	int i=100;
	while(i--) addrow();
	while(J.cline->nx != NULL){ 
		char s[10]; snprintf(s,10,"%d", J.cline->idx);
		strcpy(J.cline->s, s);
		J.cline->len = strlen(s);
		
		J.cline=J.cline->nx;
	}
	J.cline = J.fline;
	while((J.cline = J.cline->nx)->idx < 10);

	char s[1000] = "123456789101112131415161718192021222324252627282930313233343536373839404142431234567891011121314151617181920212223242526272829303132333435363738394041424312345678910111213141516171819202122232425262728293031323334353637383940414243";
	J.cline->s = s;
	J.cline->len = strlen(s);
	J.cline = J.fline;
	
	while(1){
		kbinput();
		drawscreen();
//		fprintf(stderr, cpos+1);
//		fprintf(stderr, "\nJ.cx, J.cy %d,%d", J.cx,J.cy);
	}


	exitraw(STDIN);
}	



