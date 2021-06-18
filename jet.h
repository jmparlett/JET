/********************************************************
 * 		DEFINES AND OTHER JUNK			*
 * ******************************************************/

#define TABSIZE 8 /* number of spaces in a tab */


/*******KEY CODES*******/
enum editorkeys{
	QUIT = 17, /* ctr-q */

	BKSPC = 127, /* backspace */
	
	ENTER = 13,
	
	TAB = 9,


};


/********************************** Editor Structure *****************************/
typedef struct frow{ /* file row struct */

	int idx; /* line num in file */
	
	int len; /* length of line */

	int mlen; /* max length */
	
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



/*************** UTIL FUNCS/MACROS *******************/
#define typeable(A) ((A) >= 32 && (A) < 127)

int extendline(frow *line, int len){/* extend current line by a given len (num of chars) */
	line->mlen += len;

	if((line->s = (char *)realloc(line->s, line->mlen)) == NULL) return -1; 

	return 0;
}


