# JET Text Editor Project

Screen buffer: Need a method to detect how many screens rows and screen columns. 

File buffer: Need a file/text buffer to read/write text from.

Cursor: it should move between screen rows and columns. If at top or bottom of screen then next line should be pulled from file buffer and previous rows in frame buffer should be moved back.


# Screen Buffer
should be navigatable with an (x,y) system. Like a 2d array.

screen buffer: based on position in file and number of rows a range of rows from the file will be written to STDOUT
	       .  This can be one fat string of characters from a rowobj. You must also write a carrige return and
		a newline at the end of each file row. if file buf is empty then "~\r\n" will be written in th
		e place of empty rows.

		line to long: if x is at an off screen position start drawing at a (colc * x//colc)
				

file buffer: contains all lines of file and is what will actually be edited. Upon saving filebuffer should be
written to file.

Empty behavior: Upon typeing the input handleing function should create a new row and append it to
	     the file buf.

file buf will be a linked list of row objects. Each row will contain a string of len col count.
this will be doubled if they go beyond col count. Upon entering a newline "enter" a new node should
added to the list. Upon deletion of a line that node will be removed and all nodes in front of it
will have their index incremented by 1.	    

	drawscreen():
		storecursor position
		move cursor to home 
		starting at top line on screen draw to bottom line on screen
		set cursor to previous position

	Line truncation:
		use a global var to tell what position to start drawing a line;
		if x > then col count but < line len then this global is incremented 
		when moving to a new line this can be set to 0 or we can start at x=line len and global var = line len

		for drawing starting at a certain position in the line if (global var + col count) > len of line then draw nothing
		else draw (line ptr + (global var + col count)) which should be (len - (global var + col count))



# Writing to lines

line->len must ALWAYS be accurate or stuff will break!!!!

1. inserting chars
	c = printable char
	s[x] = current position
	x = index
	if x+1 > len of line then len realloc(line, len+colc)
	given c, if s[x] is empty s[x] = c and x+=1, len+=1
		if s[x] is not empty then insert c at pos x and push all chars forward in array
		x+=1, len+=1

backspace

	if x==0 and len == 0 then move to previous lines last char and remove current line from linked list.

	elif x==0 them append line to previous line and remove line; move x to last char of new line.

	else
	x-=1; len-=1; remove [s];

first two cases will be handled by remove line backspace will simply callit

 delete

	similar behavior to backspace removes a line if x=len-1 (last position)
	else
	len-=1 remove s[x+1]


enter

	if x-1 == len then add an empty line and position cursor at start of line

	else split the current line at position x, insert a new line below and copy right part of line to new line

	addline() will handle these cases enter pretty much only calls addline

tab 

	how do you deal with tabs :( whyyyyyyy.
	taboff(): calculate tabs to the left of x and return an appropriate offset value (on my system tabs = 5 spaces)
	"The traditional meaning of TAB is to move the cursor to the next multiple of tab width (8 by the Java spec)." What?
	So this means we need a function to calculate tab offset. We can do this by faking tabs as space in a func.
	tc = tabcount f = current offset i = index of current tab
	for( char c in array )
		if c == '\t' (calc tabs index + current offsets distance to next multiple of tab)
			t = i
			while(t+f % 8 != 0) ++t;
			f += t-i;
			tc++;
	this should done only for tabs behind current position
	
	after accounting for all the weirdness of how I chose to render the cursor this is the final algorithm in C
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

# Cursor
Cursor snapping will always snap to end of line ready to type

how to move the cursor: 
		ts = top line on screen; bs = bottom line on screen; cl = current line.

		need to use ansi escape codes written to console

		when cursor moves left: decrease x position by 1 while x greater then 0
				
				if x == 0 then move cursor up one line and set x to last char in line
				cl = cl->prev
				x = line len -1

		when cursor moves right: increase x by 1 while x < col count
				if x == line len -1 then move cursor down one line and set x to first char in line
				cl = cl->nx
				x = 0

		when cursor moves up decrease y by 1 while y greater then 0. If y becomes less than screen top then scroll up.
				to scroll up ts=ts->prev and bs=bs->prev.	
				cl=cl->prev
				

		when cursor moves down increase y by 1 while y < line count
				to scroll down ts=ts->next and bs=bs->next.	
				cl=cl->next


# File buffer
## Loading a file into the file rows linked list structure via args
1. add first line with addline(); getchars from file and call insertchar() on current/first line.
2. when '\n' is encountered call addline() and step to next line cline = cline->next
3. getchars from file and call insertchar() on current line
4. repeat process until EOF is encountered
5. set cline and tsl (top screen line) to first line

## writing the contents of the file rows structure to a file
1. starting at fline
2. while cline->next != null
3. step through chars in line putchar(line[i]) to file until i == le
4. then putchar('\n') to file and step to next line; cline = cline->next

## appropriate confirmations (are you sure you want to save, etc)

1. prompting for filename when saving a new file (opened the editor without a file arg)
if they have a file opened display status msg saying <filename> saved.
if they opened without a file prompt for a file name to save then display status msg.



# Beutifications

1. line numbers (maybe)

2. status bar (at top shows file name)

3. help menu (shows controls) (also a maybe)

4. msg bar (at bottom displays prompts for saving files etc





# Bugs
1. Scrolling through truncated lines containing tabs positions the cursor in weird ways.
2. A newline is added to the added of the file when it is saved regardless of weather or not there was one.
	
# Future plans
1. Implement line deletion
2. Implement copy paste
3. Implement Block deletion and copy pasting
4. Better scrolling
	a. move to top/bottom of doc
	b. scroll page by page (a full screens space)
5. debug linked list of rows (need to free the sub allocations of each line)
