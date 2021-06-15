_____________________JET Text Editor Project________________________

Screen buffer: Need a method to detect how many screens rows and screen columns. 

File buffer: Need a file/text buffer to read/write text from.

Cursor: it should move between screen rows and columns. If at top or bottom of screen then next line should be pulled from file buffer and previous rows in frame buffer should be moved back.


Libs:
	Termios



_____________________Screen Buffer_____________________
should be navigatable with an (x,y) system. Like a 2d array.

screen buffer: based on position in file and number of rows a range of rows from the file will be written to STDOUT
	       .  This can be one fat string of characters from a rowobj. You must also write a carrige return and
		a newline at the end of each file row. if file buf is empty then "~\r\n" will be written in th
		e place of empty rows.

		line to long: attempt to split and advance the count until whole line is written. Break if reached
			      max rows.
		

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
move cursor to home (0,0)
starting at current row in file buff write row to STDOUT followed by "\r\n" if no row is present write "~\r\n"
stop at current row + editorrows
set cursor to previous position
_______________Keyboard input______________________________ 



_______________Cursor__________________________________

how to move the cursor: 
		
		ANSI Escape code: Do i need to be in raw for this?





_______________Notes______________________
rows 33, cols 120

Note: the screen rows buffer structure must be entirely different from file structure as you will be writeing more text than the screen rows hold.


