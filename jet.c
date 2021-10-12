#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "kbgetch.h"
#include "jet.h"

#define STDIN STDIN_FILENO
#define STDOUT STDOUT_FILENO
#define STERR STDERR_FILENO
#define SCRDIMBYTES 100 /* used in kbinput() to capture cursor x and y as a string. Must be able to capture the max screen dimensions */
#define UP 1000
#define DOWN 1001
#define TITLEMSG "JET Text Editor"

static struct termios save_term; /* previous term settings */
static int termsaved; /* status of function */
char cpos[SCRDIMBYTES];  /* string used for cursor position */
static editor J; /***** IMPORTANT!!! main editor obj *****/
static char *cfname; /* name of current file */
static char *statusmsg; /* current status msg */ 
static char *titlemsg; /* current title msg which will be set to the file name if one is present */
static char conqkey[] = " Ctrl-Q: quit	Ctr-S: save file ESC: cancel";

/*****PROTOS******/
void insertchar(frow *, char);
void removechar(frow *);
void backspace(void);
void enter(void);
int taboff(char *, int);
int drawscreen(void);
int loadfile(char []);
int savefile(char []);


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

int exitraw(int fd){ /* set term to vals saved at beginning of program */
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

void updatetitlebar(){
	

	titlemsg = malloc(J.colc);
	
	if(cfname != NULL){ /* draw file name in title bar */
	
	int fill = (J.colc / 2) - (strlen(cfname)/2); /* begining space fill for file name */

	int i, j;	
	for(i = 0;  i < fill; ++i) titlemsg[i] = ' ';
	
	for(j = 0; j < strlen(cfname); ++j, ++i) titlemsg[i] = cfname[j];

	for(; i < J.colc; ++i) titlemsg[i] = ' ';

	} else { /* draw editor name in title bar */
		char ename[] = TITLEMSG; /* just using a defines so we can change the msg later if we want to */

		int fill = (J.colc / 2) - (strlen(ename)/2); /* begining space fill for file name */

		int i, j;	
		for(i = 0;  i < fill; ++i) titlemsg[i] = ' ';
		
		for(j = 0; j < strlen(ename); ++j, ++i) titlemsg[i] = ename[j];

		for(; i < J.colc; ++i) titlemsg[i] = ' ';

	}
}

void drawtitlebar(scrbuf *buf){

	scrbufappend(buf ,"\x1b[0K", 4);

	scrbufappend(buf ,"\x1b[7m", 4);

	scrbufappend(buf , titlemsg, J.colc);

	scrbufappend(buf ,"\x1b[0m\r\n", 6);

}
void updatestatusmsg(char *msg){
	
	/* status: <msg> */
	if(msg != NULL){
		statusmsg = (char *)calloc(J.colc, sizeof(char));
		int i;

		strcpy(statusmsg, " Status: ");
		if(strlen(msg) < (J.colc - strlen(statusmsg))) strcat(statusmsg, msg);
		else{ /* print error */
			char err[] = "error: status msg too long";
			strcat(statusmsg, err);
		}

		for(i=strlen(statusmsg); i < J.colc; ++i) statusmsg[i] = ' ';
	} else { /* draw empty bar, we can use this to clear old msgs */
		statusmsg = (char *)calloc(J.colc, sizeof(char));
		int i;

		strcpy(statusmsg, " Status: ");

		for(i=9; i < J.colc; ++i) statusmsg[i] = ' ';

	}
}


void drawstatusbar(scrbuf *buf){

	scrbufappend(buf ,"\x1b[0K", 4);

	scrbufappend(buf ,"\x1b[7m", 4);

	scrbufappend(buf , statusmsg, J.colc);

	scrbufappend(buf ,"\x1b[0m\r\n", 6);


}

int promptformsg(char p[], char r[], int len){ /* p = prompt, r = place to store response, len = max len of response*/
	
	int i, j, c; 
	i = c = 0;
	j = strlen(p);


	char bufmsg[j + len]; /* holds response plus msg to print to user */
	
	strcpy(bufmsg, p);

	for(int u=j; u < (j+len); ++u) bufmsg[u] = '\0'; /* zero what we display so we dont get weird chars */
	
	updatestatusmsg(bufmsg); /* draw initial prompt */
	drawscreen();

	while(c != ENTER && c != ESC && i < len){
		read(STDIN, &c, 1);
		if(typeable(c)) r[i++] = bufmsg[j++] = c;
		updatestatusmsg(bufmsg);
		drawscreen();
	
	}
	r[i] = '\0'; /* delimit string */
	if( c == ESC) return ESC;

	else return 0;

}

void drawconqkey(scrbuf *buf){

	scrbufappend(buf ,"\x1b[0K", 4);

	scrbufappend(buf , conqkey, strlen(conqkey));

	scrbufappend(buf ,"\x1b[0m\r\n", 6);

}

int drawscreen(){
	scrbuf buf = {0, NULL};
	
	int taboffset =	(J.cline != NULL ? taboff(J.cline->s, J.cx) : 0); /* calculate normal offset */	

	write(STDOUT, "\x1b[2J", 4); /* clear screen */
	write(STDOUT, "\x1b[H", 3); /* move cursor home */

	drawtitlebar(&buf);

	int i=0;

	frow *t = J.tsl; /* file row pointer */

	for(; i < J.rowc-5; ++i){
		if(t != NULL){
			if(J.cx > J.colc && t == J.cline){/* need to scroll a truncated line */
				
				int off = (J.cx / J.colc) * J.colc;
			
				taboffset = taboff(t->s+off, J.cx-off); /* need to recalculate tab offset if we are on a truncd line */
				
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

	drawstatusbar(&buf);
	drawconqkey(&buf);
	write(STDOUT, buf.c, buf.len);
	freescrbuf(buf);

	 /* draw cursor position wrapping x around screen */
	snprintf(cpos, SCRDIMBYTES, "\x1b[%d;%dH", J.cy+2, ((J.cx+1)%J.colc)+taboffset);
	write(STDOUT, cpos, strlen(cpos));

	return 0;



}

void scroll(int direction){ /* general function to screen based on current y. If not we increment or decrement y */
	if(direction == UP){
		if(J.cy == 0) J.tsl = J.tsl->pr;
		else J.cy--;
	} else if(direction == DOWN){
		if(J.cy == J.rowc-6) J.tsl = J.tsl->nx;
		else J.cy++;
	}
}

/*********************************** KEYBOARD INPUT ***************************************/
void kbinput(){
	char b = kbgetch();
	
	if(b == '\x1b'){ /* we have an escape */
		if((b=kbgetch()) != '['){ /* it was a single escape we want to ungetch and call kbinput to handle */
			ungetch(b);
			kbinput();
		}
		else{ /* we have an escape sequence to handle */
			b = kbgetch();
			switch(b){
				case 'A': /* move up */
				if(J.cline->pr != NULL){
					if(J.cline->len > J.cline->pr->len) J.cx = J.cline->pr->len; /* snap to end of line if current is longer */
					J.cline = J.cline->pr;
					scroll(UP);
									
				}
				break;
				case 'B': /* move down */
				if(J.cline->nx != NULL){
					if(J.cline->len > J.cline->nx->len) J.cx = J.cline->nx->len; /* snap to end of line if current is longer */
					J.cline = J.cline->nx;
					scroll(DOWN);
				}
				break;
				case 'C': /* move right */
				if(J.cx == J.cline->len  && J.cline->nx != NULL){ /* move to next line */
					J.cline = J.cline->nx;
					scroll(DOWN);
	
					J.cx = 0; /* first index */
				} else if(J.cx < J.cline->len) {
					J.cx++;
				}
				break;
				case 'D': /* move left */
				if(J.cx == 0 && J.cline->pr != NULL){ /* move to prev line */
					J.cline = J.cline->pr;
					scroll(UP);
					J.cx = J.cline->len; /* snap to end of line ready to insert chars */

				} else if(J.cx > 0){
				 J.cx--;
				}
				break;
			
				case 'F': /* end */
					J.cx = J.cline->len;	
				break;

				case 'H': /* home */
					J.cx = 0;	
				break;
				
				

			}
		}
	} else if(typeable(b)){ /* add character to page... typing stuff goes here */
		insertchar(J.cline, b);
	
	} else if(b == BKSPC) { /* delete character or lines if used at beginning of line standard backspace stuff */
		backspace();

	} else if (b == ENTER){ /* do enter things...*/
		enter();
		
	
	} else if(b == TAB){
		insertchar(J.cline, '\t');
		
	} else if(b == SAVE){
		if(cfname != NULL) { /* we have a file opened */
		
			savefile(cfname); /* save current file */

			char savemsg[strlen(cfname)+7];

			strcpy(savemsg, cfname);

			strcat(savemsg, " saved");

			updatestatusmsg(savemsg);

			updatetitlebar(); /* display new filename */


		} else { /* editor was started without file so prompt for name to save */
						
			cfname = (char *)malloc(100);
	
			if(promptformsg("filename to save? ", cfname, 100) == ESC){ /* cancel */
				cfname = NULL;
				updatestatusmsg("save canceled");
			} else {
				
				savefile(cfname); /* save new file */

				char savemsg[strlen(cfname)+7];

				strcpy(savemsg, cfname);

				strcat(savemsg, " saved");

				updatestatusmsg(savemsg);

				updatetitlebar(); /* display new filename */
			}

					
		}
	
	} else if(b == QUIT){
		write(STDOUT, "\x1b[2J", 4); /* clear screen */
		write(STDOUT, "\x1b[H",3); /* send cursor home */
		exitraw(STDIN);
		exit(0);
	}


}
/************************Writing******************************/


void addline(void){
	if(J.fline == NULL){ /* general use function for adding line node to list */
		frow *p = (frow *)malloc(sizeof(frow));	
		p-> idx = 0;
		p->nx = NULL;
		p->pr = NULL;
		p->len = 0;
		p->mlen = J.colc;
		p->s = (char *)calloc(J.colc, sizeof(char));
		J.fline = J.cline = J.tsl = p;
		
	} else { /* add new row after J.cline (current line) */
		
		frow *new = (frow *)malloc(sizeof(frow));
		new->idx = J.cline->idx+1;
		new->nx = J.cline->nx;
		new->pr = J.cline;
		new->mlen = J.colc;
		new->s = (char *)calloc(J.colc, sizeof(char));

		new->len = 0;
		J.cline->nx = new;
		if(new->nx != NULL){
			new->nx->pr = new;
			frow *t=new;
			while((t=t->nx) != NULL) t->idx++; /* adjust index of everything in front */
		}
	}
	return;

}

void deleteline(frow *line){ /* general use function for removing line node from list */
	/* adjust index of everything in front */	
	frow *t = line;
	while((t = t->nx) != NULL) t->idx--;
	if(line->pr == NULL){/*deleting head */
		J.fline = line->nx;	
	} else if(line->nx == NULL){ /* deleting tail */
		line->pr->nx = NULL;
	} else { /* deleting internal node */
		line->pr->nx = line->nx;
		line->nx->pr = line->pr;
	}
	free(line);
}

void enter(void){ /* used only for enter not a general function */
	
	if(J.fline == NULL){ /* no lines yet so add first line */	
		addline();
		J.cx = J.cy = 0;
	} else if(J.cx == J.cline->len-1){ /* at end of a line so add a new one */
		addline();
		J.cline = J.cline->nx;
		scroll(DOWN);
		J.cx=0;
		
	} else { /* copy everything from cursor to end of line to new line inserted below then delete moved chars */
		addline();
		int t = J.cx, i; /* remember position for deletion later */
		for(i=t, J.cx=0; i < J.cline->len; ++i) insertchar(J.cline->nx, J.cline->s[i]); /* copy chars */
		for(; --J.cline->len >= t; J.cline->s[J.cline->len] = 0); /* delete chars */
		J.cline->len++; /* after loop len is value of last index. Should be one above index */
		J.cline = J.cline->nx;
		scroll(DOWN);
		J.cx=0;
	}


}




void backspace(void){ /* not a generally useable function do not use out side of backspace */
	if(J.cx == 0 && J.cline->pr != NULL){/* if at back of line and there is a previous line */		
		J.cline = J.cline->pr;

		int t = J.cx = J.cline->len; /* kinda jank but we have to set this so insertchar works correctly 
						   also record it so we can set x to the joined point of the lines*/


		if(J.cline->nx->len == 0) { /* if empty delete */

			deleteline(J.cline->nx);

		} else { /* append text to previous line then delete */

			for(int i=0; i < J.cline->nx->len; ++i) insertchar(J.cline, J.cline->nx->s[i]);

			deleteline(J.cline->nx);	
		}

		J.cx = t;
		scroll(UP);
 	} else if(J.cline->len > 0 && J.cx > 0){
		/* delete character behind cursor and move cursor back one and decrement line count by 1*/
		for(int i=J.cx; i < J.cline->len; i++) J.cline->s[i-1] = J.cline->s[i];
		J.cx--;
		J.cline->len--;
	}
}

/* add a character into the current line at the current position */
void insertchar(frow *line, char c){
	
	if(line->len+1 == line->mlen) extendline(line, J.colc);/* if about to overflow alloc another screens worth of space for line */

	if(!typeable(line->s[J.cx])){
		line->s[J.cx] = c;
		line->len++;
		J.cx++;
	} else {
		int i=line->len; /* index of position beyond last char in str. Recall we have already ensured we have enough space */
		while(i > J.cx){
			line->s[i] = line->s[i-1]; 
			i--;
		}	
		line->s[J.cx] = c;
		line->len++;
		J.cx++;
	}

}


int taboff(char *s, int x){ /* calculate tab offset for rendering cursor */
	int f, i, t;
	f = i = 0;
	while(i < x){
		if(s[i] == '\t'){
			 t = i+1;
			 while((t+f) % TABSIZE != 0 ) t++; /* dont forget %'s precedence*/
			 f += t-(i+1);
		}
		i++;
	}
	return f;
}



/**********************FILE MANAGEMENT*****************************/

int loadfile(char fname[]){ /* to be called at the start of the program if a file name is passed as an argument */
	FILE *fp;

	if((fp = fopen(fname, "r")) == NULL) return -1;

	int c;

	addline();  /* add first line (this sets cline and fline as well) */

	while((c = fgetc(fp)) != EOF){
	
		if(c == '\n'){
			J.cx = 0;
			addline();
			J.cline = J.cline->nx;

		} else {
			insertchar(J.cline, c);
		}	

	}

	J.cline = J.tsl = J.fline;
	
	fclose(fp);

	return 0;
}


int savefile(char fname[]){
	
	FILE *fp;

	if((fp = fopen(fname, "w")) == NULL){ /* need to save a new file */

		
	} else {
		frow *t = J.fline; /* start at top of file */

		while(t != NULL){

			for(int i=0; i < t->len; ++i) fputc(t->s[i], fp);
			fputc('\n',fp);
			t = t->nx;
		}
		return 0;
	}
	return -1;

	

}

int main(int argc, char *argv[])
{
	/* enter raw and initialize the editor */
	enterraw(STDIN);
	initeditor();


	if(argc > 1){ /* attempt to open a file and read it into the editor */
		loadfile((cfname = *++argv));		
	} else{
		cfname = NULL; /* no file name */
		addline(); /* temp fix so the program doesnt crash if you press anything other
						  than enter when you open without a filename arg */
	}


	updatetitlebar();
	updatestatusmsg(NULL);
	
	
	while(1){
		drawscreen();
		kbinput();
	}
}	



