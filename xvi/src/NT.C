/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    nt.c
* module function:
    System interface routines for Windows NT.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

#include "xvi.h"

#undef va_start
#undef va_end

#include <windows.h>

#define PERR(r,api) {if(!(r)) perr(__FILE__, __LINE__, api, GetLastError());}
#define BACKGROUND_WHITE (BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE)
#define FOREGROUND_WHITE (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE)

void perr(PCHAR, int, PCHAR, DWORD );
void setconssize(HANDLE ,int ,int );
int getConX(HANDLE);
int getConY(HANDLE);
void settextcolor(void);
int virtkey(KEY_EVENT_RECORD);

int Rows, Columns;

static COORD Currpos;
static SMALL_RECT WindowRect;
static COORD WindowSize;
static HANDLE hStdIn, hStdOut;
static WORD Currcolor = FOREGROUND_GREEN;

void
popupmsg(char *s)
{
	perr(__FILE__, __LINE__, s, 0 );
}

BOOL
myhandler(DWORD dwCtrlType)
{
	switch(dwCtrlType){

	case CTRL_C_EVENT:
		/* used to be: kbdintr = 1; */
		(void) do_preserve();
		exit(0);
		return TRUE;

	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
		(void) do_preserve();
		exit(0);
		return TRUE;
	}
	return FALSE;
}

void
ignore_signals(void)
{
	SetConsoleCtrlHandler((PHANDLER_ROUTINE) &myhandler,FALSE);
}

void
catch_signals(void)
{
	SetConsoleCtrlHandler((PHANDLER_ROUTINE) &myhandler,TRUE);
}

void
flush_output(void)
{
}

void
alert(void)
{
	Beep(600,200);
}

void
sys_init(void)
{
	BOOL r;
	DWORD dwMode;

	r = FreeConsole();
	if ( !r ) {
		Beep(600,200);
		Beep(1200,200);
		exit(1);
	}

	r = AllocConsole();
	if ( !r ) {
		Beep(1200,200);
		Beep(600,200);
		exit(1);
	}
	r = SetConsoleTitle("XVI - Windows NT Version");
	PERR(r,"SetConsoleTitle");

	PERR(1,"PERR TEST");

	/* Get standard Handles */

	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	PERR(hStdOut != INVALID_HANDLE_VALUE, "GetStdHandle");

	hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	PERR(hStdIn != INVALID_HANDLE_VALUE, "GetStdHandle");

	/* set up mouse and window input */
	r = GetConsoleMode(hStdIn, &dwMode);
	PERR(r, "GetConsoleMode");

	/* when turning off ENABLE_LINE_INPUT, you MUST also turn off */
	/* ENABLE_ECHO_INPUT. */
	r = SetConsoleMode(hStdIn, (dwMode & ~(ENABLE_LINE_INPUT |
		ENABLE_ECHO_INPUT))| ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
	PERR(r, "SetConsoleMode");

	setconssize(hStdOut,80,24);
}

void
setconssize(HANDLE hConsole,int xsize,int ysize)
{
	WORD wConBufSize;
	BOOL r;

	Rows = ysize;
	Columns = xsize;

	/* The order of resizing buffer vs. resizing window is dependent on */
	/* whether we're currently larger or smaller than the desired size. */

	wConBufSize = getConX(hConsole) * getConY(hConsole);
	WindowRect.Left = 0;
	WindowRect.Top = 0;
	WindowRect.Right = xsize-1;
	WindowRect.Bottom = ysize-1;
	WindowSize.X = xsize;
	WindowSize.Y = ysize;
	
	if ( wConBufSize > (xsize*ysize) ) {
		r = SetConsoleWindowInfo(hConsole, TRUE, &WindowRect);
		PERR(r,"SetConsoleWindowInfo");
		r = SetConsoleScreenBufferSize(hConsole, WindowSize);
		PERR(r,"SetConsoelScreenBufferSize");
	}
	else if ( wConBufSize < (xsize*ysize) ) {
		r = SetConsoleScreenBufferSize(hConsole, WindowSize);
		PERR(r,"SetConsoelScreenBufferSize");
		r = SetConsoleWindowInfo(hConsole, TRUE, &WindowRect);
		PERR(r,"SetConsoleWindowInfo");
	}

	erase_display();
	set_colour(1);
	settextcolor();
}

void
settextcolor()
{
	int r;

	/* set attributes for new writes */
	r = SetConsoleTextAttribute(hStdOut,Currcolor);
	PERR(r,"SetConsoleTextAttribute");
}

void
outchar(int c)
{
	char s[2];
	s[0] = c;
	s[1] = '\0';
	outstr(s);
}

void
outstr(char *s)
{
	DWORD cCharsWritten;
	int r;

	r = WriteFile(hStdOut,s,strlen(s),&cCharsWritten, NULL);
	PERR(r,"WriteFile(hStdOut...)");
	
}

void
erase_display(void)
{
	int r;
	COORD pos = { 0, 0 };
	DWORD nwritten;

	r = FillConsoleOutputAttribute(hStdOut, 0,
		WindowSize.X*WindowSize.Y, pos, &nwritten);
	PERR(r,"FillConsoleOutputAttribute");
}

void
erase_line(void)
{
	int nblanks;
	int r;
	DWORD nwritten;

	nblanks = WindowSize.X - Currpos.X;
	r = FillConsoleOutputAttribute(hStdOut,0,nblanks,Currpos,&nwritten);
	PERR(r,"FillConsoleOutputAttribute");
}

void
tty_goto(int r, int c)
{
	int ret;

	Currpos.X = c;
	Currpos.Y = r;
	ret = SetConsoleCursorPosition(hStdOut,Currpos);
	PERR(ret,"SetConsoleCursorPosition");
}

int
getConX(HANDLE hcon)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	BOOL r;

	r = GetConsoleScreenBufferInfo(hcon, &csbi);
	PERR(r, "GetConsoleScreenBufferInfo");
	return(csbi.dwSize.X);
}

int
getConY(HANDLE hcon)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	BOOL r;

	r = GetConsoleScreenBufferInfo(hcon, &csbi);
	PERR(r, "GetConsoleScreenBufferInfo");
	return(csbi.dwSize.Y);
}

static	enum {
	m_SYS = 0,
	m_VI
}	curmode;

/*
 * Set up video state for editor.
 */
void
sys_startv(void)
{
}

/*
 * Restore video state to what it was when we started.
 */
void
sys_endv(void)
{
}

void
sys_exit(int r)
{
    sys_endv();
    exit(r);
}

void
sleep(n)
unsigned	n;
{
	flush_output();
	_sleep(n*1000);
}

void
delay()
{
    clock_t start = clock();

    flush_output();
    while (clock() < start + CLOCKS_PER_SEC / 5)
	;
}

/*
 * This function is only used by tempfname(). It constructs a filename
 * suffix based on an index number.
 *
 * The suffix ".$$$" is commonly used for temporary file names on
 * MS-DOS & OS/2 systems. We also use the sequence ".$$1", ".$$2" ...
 * ".fff" (all digits are hexadecimal).
 */
static char *
hexsuffix(i)
unsigned	i;
{
    static char	suffix [] = ".$$$";
    static char	hextab [] = "0123456789abcdef";
    char*		sp = &suffix[3];

    while (sp > suffix) {
	if (i > 0) {
	    *sp-- = hextab [i & 0xf];
	    i >>= 4;
	} else {
	    *sp-- = '$';
	}
    }
    return suffix;
}

/*
 * Construct unique name for temporary file, to be used as a backup
 * file for the named file.
 */
char*
tempfname(srcname)
char		*srcname;
{
    char	*srctail,
		*srcdot,
		*endp,
		*retp;
    unsigned	indexnum = 0;
    unsigned	baselen;

    srctail = srcdot = NULL;
    endp = srcname;

    while (*endp) {
	switch (*endp++) {
	case '\\':
	case '/':
	    srctail = endp;
	    srcdot = (char*) 0;
	    continue;
	case '.':
	    srcdot = endp - 1;
	}
    }

    if (srctail == NULL) {
	/*
	 * We haven't found any directory separators ('/' or '\\').
	 */
	srctail = srcname;
	/*
	 * Check to see if there's a disk drive name. If there
	 * is, skip over it.
	 */
	if (*srcname && is_alpha(*srcname) && srcname[1] == ':') {
	    srctail = &srcname[2];
	}
    }

    /*
     * There isn't a dot in the trailing part of the filename:
     * just add it at the end.
     */
    if (srcdot == NULL) {
	srcdot = endp;
    }

    /*
     * Don't make name too long.
     */
    if (srcdot - srctail > MAXNAMLEN - 4)
	srcdot = srctail + MAXNAMLEN - 4;
    if (srcdot - srcname > MAXPATHLEN - 4)
	srcdot = srcname + MAXPATHLEN - 4;
    baselen = srcdot - srcname;
    /*
     * Allocate space for new temporary file name ...
     */
    if ((retp = alloc(baselen + 5)) == NULL)
	return NULL;
    if (baselen > 0)
	(void) memcpy(retp, srcname, baselen);
    do {
	/*
	 * Keep trying this until we get a unique file name.
	 */
	strcpy(&retp[baselen], hexsuffix(indexnum++));
    } while (exists(retp));
    return retp;
}

/*
 * Fake out a pipe by writing output to temp file, running a process with
 * i/o redirected from this file to another temp file, and then reading
 * the second temp file back in.
 */
bool_t
sys_pipe(cmd, writefunc, readfunc)
char	*cmd;
int	(*writefunc) P((FILE *));
long	(*readfunc) P((FILE *));
{
    char	*temp1;
    FILE	*fp;
    bool_t	retval;

    /*
     * Create first temporary file ...
     */
    if ((temp1 = tempfname("xvi_out")) == NULL ||
    				(fp = fopen(temp1, "w")) == NULL) {
	retval = FALSE;
    } else {
	char	*temp2 = NULL;
	int	savcon;
	int	fd1 = -1,
		fd2 = -1;

	/*
	 * ... then write to it & close it ...
	 */
	(void) (*writefunc)(fp);
	(void) fclose(fp);

	/*
	 * ... then re-open it for reading, open second one
	 * for writing & re-arrange file descriptors.
	 *
	 * Note that we assume that the editor's standard
	 * input, output & error files are the same device,
	 * since I can't imagine how any of them could
	 * usefully be redirected to anything else.
	 */

#ifndef _O_BINARY
#	define _O_BINARY 0
#endif
#ifndef _O_EXCL
#	define _O_EXCL 0
#endif
	if (
	    (savcon = dup(0)) < 3
	    ||
	    (fd1 = open(temp1, _O_RDONLY | _O_BINARY)) < 3
	    ||
	    (temp2 = tempfname("xvi_in")) == NULL 
	    ||
	    (fd2 = open(temp2, _O_WRONLY|_O_CREAT|_O_EXCL|_O_BINARY, 0600)) < 3
	) {
	    retval = FALSE;
	} else {
	    (void) dup2(fd1, 0);
	    (void) dup2(fd2, 1);
	    (void) dup2(fd2, 2);

	    (void) close(fd1);
	    (void) close(fd2);
	    fd1 = fd2 = -1;

	    /*
	     * Run the command.
	     */
	    (void) system(cmd);

	    /*
	     * Restore our standard input, output & error
	     * files.
	     */
	    (void) dup2(savcon, 0);
	    (void) dup2(savcon, 1);
	    (void) dup2(savcon, 2);

	    /*
	     * Now read from the second temporary file,
	     * close it, & we're done.
	     */
	    if ((fp = fopen(temp2, "r")) == NULL) {
		retval = FALSE;
	    } else {
		(void) (*readfunc)(fp);
		(void) fclose(fp);
		retval = TRUE;
	    }
	}
	/*
	 * Clean up.
	 */
	if (temp2) {
	    (void) remove(temp2);
	    free(temp2);
	}
	if (savcon > 2)
	    (void) close(savcon);
	if (fd1 > 2)
	    (void) close(fd1);
	if (fd2 > 2)
	    (void) close(fd2);
    }

    if (temp1) {
	(void) remove(temp1);
	free(temp1);
    }

    return(retval);
}

/*
 * The following functions are untested because neither of us has
 * access to an MS-DOS compiler at the moment.
 */

/*
 * Expand environment variables in filename.
 */

#define VMETACHAR	'$'

static char *
vexpand(name)
    char		*name;
{
    static Flexbuf	b;
    register char	*p1, *p2;

    if ((p2 = strchr(name, VMETACHAR)) == (char *) NULL) {
	return name;
    }
    flexclear(&b);
    p1 = name;
    while (*p1) {
	register int	c;
	register char	*val;
	Flexbuf		vname;

	while (p1 < p2) {
	    (void) flexaddch(&b, *p1++);
	}
	flexnew(&vname);
	for (p2++; (c = *p2) != '\0' && (is_alnum(c) || c == '_'); p2++) {
	    (void) flexaddch(&vname, c);
	}
	if (!flexempty(&vname)
	    &&
	    (val = getenv(flexgetstr(&vname))) != (char *) NULL) {
	    while ((c = *val++) != '\0') {
		(void) flexaddch(&b, c);
	    }
	    p1 = p2;
	}
	flexdelete(&vname);
	if ((p2 = strchr(p1, VMETACHAR)) == (char *) NULL) {
	    while ((c = *p1) != '\0') {
		(void) flexaddch(&b, c);
		p1++;
	    }
	}
    }
    return flexgetstr(&b);
}

int
inchar(long mstimeout)
{
	DWORD timedout;

	timedout = WaitForSingleObject(hStdIn,mstimeout==0 ? INFINITE : (DWORD)mstimeout);
	if ( timedout != 0 )
		return EOF;
	else {
		INPUT_RECORD rec;
		DWORD nread;
		BOOL r;

		r = ReadConsoleInput(hStdIn,&rec,1,&nread);
		switch(rec.EventType) {
		case KEY_EVENT:
			return virtkey(rec.Event.KeyEvent);
			break;
		case MOUSE_EVENT:
			break;
		case WINDOW_BUFFER_SIZE_EVENT:
			break;
		case MENU_EVENT:
			break;
		case FOCUS_EVENT:
			break;
		}
		return EOF;
	}
}

int
myGetchar(void)
{
	CHAR chBuf;
	DWORD dwRead;
	int r;

	r = ReadFile(hStdIn, &chBuf, sizeof(chBuf), &dwRead, NULL);
	if ( !r )
		return -1;
	else
		return (int)chBuf;
}

/********************************************************************
* FUNCTION: perrError(void)                                         *
*                                                                   *
* PURPOSE: beep twice and abort program                             *
*                                                                   *
* INPUT: none                                                       *
*                                                                   *
* RETURNS: none                                                     *
*                                                                   *
* COMMENTS: only called from perr() when a fatal error has occurred *
********************************************************************/

void perrError(void)
{
  DWORD dwLastError; /* for debugger purposes */

  dwLastError = GetLastError();
  /* we can't output an error message - beep instead */
  Beep(600, 200);
  Beep(700, 200);
  exit(1);
}


/*********************************************************************
* FUNCTION: perr(PCHAR szFileName, int line, PCHAR szApiName,        *
*                DWORD dwError)                                      *
*                                                                    *
* PURPOSE: report API errors. Allocate a new console buffer, display *
*          error number and error text, restore previous console     *
*          buffer                                                    *
*                                                                    *
* INPUT: current source file name, current line number, name of the  *
*        API that failed, and the error number                       *
*                                                                    *
* RETURNS: none                                                      *
*********************************************************************/

/* this is the size of the buffer for the error message text */
#define MSG_BUF_SIZE 512

void perr(PCHAR szFileName, int line, PCHAR szApiName, DWORD dwError)
{
  BOOL bSuccess;
  HANDLE hConTemp; /* temp console buffer to put error message on */
  HANDLE hCurrentCon; /* to save the current console so we can restore it */
  CHAR szTemp[256];
  DWORD cCharsWritten;
  DWORD dwLastError;  /* for debugging purposes */
  CHAR msgBuf[MSG_BUF_SIZE]; /* buffer for message text from system */

  /* create a handle to the current console buffer */
  hCurrentCon = CreateFile("CONOUT$", GENERIC_WRITE, FILE_SHARE_READ |
     FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hCurrentCon == INVALID_HANDLE_VALUE)
    {
    dwLastError = GetLastError();
    /* just in case this works */
    printf("Fatal error in perr() on line %d: %d\n", __LINE__, dwLastError);
    Beep(600, 200);
    exit(1);
    }
  /* create a temporary console buffer that we can write on */
  hConTemp = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CONSOLE_TEXTMODE_BUFFER,
      NULL);
  if (hConTemp == INVALID_HANDLE_VALUE)
    {
    dwLastError = GetLastError();
    printf("Fatal error in perr() on line %d: %d\n", __LINE__, dwLastError);
    Beep(600, 200);
    exit(1);
    }
  /* make the temp buffer to the current buffer */
  bSuccess = SetConsoleActiveScreenBuffer(hConTemp);
  if (!bSuccess)
    {
    dwLastError = GetLastError();
    printf("Fatal error in perr() on line %d: %d\n", __LINE__, dwLastError);
    Beep(600, 200);
    exit(1);
    }
  /* set red on white text for future text output */
  SetConsoleTextAttribute(hConTemp, FOREGROUND_RED | BACKGROUND_WHITE);
  /* format our error message */
  sprintf(szTemp, "%s: Error %d from %s on line %d\n", szFileName,
      dwError, szApiName, line);
  /* write the message to the console */
  bSuccess = WriteFile(hConTemp, szTemp, strlen(szTemp), &cCharsWritten,
      NULL);
  if (!bSuccess)
    perrError();
  memset(msgBuf, 0, sizeof(msgBuf));
  /* get the text description for that error number from the system */
  cCharsWritten = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
      getConX(hConTemp), NULL, dwError, MAKELANGID(0, SUBLANG_ENGLISH_US),
      msgBuf, sizeof(msgBuf), NULL);
  if (!cCharsWritten)
    perrError();
  else
    {
    /* write the text description to the console */
    bSuccess = WriteFile(hConTemp, msgBuf, strlen(msgBuf), &cCharsWritten,
        NULL);
    if (!bSuccess)
      perrError();
    }
  myGetchar();
  /* reset the current buffer to the original one */
  bSuccess = SetConsoleActiveScreenBuffer(hCurrentCon);
  if (!bSuccess)
    perrError();
  CloseHandle(hConTemp);
  bSuccess = CloseHandle(hCurrentCon);
  if (!bSuccess)
    perrError();
  return;
}

void
scroll_down(unsigned start_row, unsigned end_row, int nlines)
{
	SMALL_RECT rect;
	COORD newcoord;
	CHAR_INFO charinfo;

	rect.Top = start_row;
	rect.Bottom = end_row;
	rect.Left = 0;
	rect.Right = WindowSize.X - 1;
	newcoord.X = 0;
	newcoord.Y = start_row + nlines;
	charinfo.Char.AsciiChar = ' ';
	charinfo.Attributes = 0;
	ScrollConsoleScreenBuffer(hStdOut,&rect,&rect,newcoord,&charinfo);
}

void
scroll_up(unsigned start_row, unsigned end_row, int nlines)
{
	scroll_down(start_row,end_row,-nlines);
}

void
set_colour(int n)
{
	int c;
	switch(n){
	case 0: c = FOREGROUND_WHITE; break;
	case 1: c = FOREGROUND_GREEN; break;
	case 2: c = FOREGROUND_BLUE; break;
	case 3: c = FOREGROUND_RED; break;
	case 4: c = FOREGROUND_RED|FOREGROUND_GREEN; break;
	case 5: c = FOREGROUND_RED|FOREGROUND_BLUE; break;
	case 6: c = FOREGROUND_GREEN|FOREGROUND_BLUE; break;
	case 7: c = FOREGROUND_BLUE; break;
	case 8: c = FOREGROUND_GREEN; break;
	case 9: c = FOREGROUND_RED; break;
	case 10: c = FOREGROUND_RED; break;
	default: c = FOREGROUND_WHITE; break;
	}
	Currcolor = c;
	settextcolor();
}

struct vkinfo {
	CHAR val;
} vktable[] = {
	0,	/* 0x00 */
	0,	/* 0x01 VK_LBUTTON */
	0,	/* 0x02 VK_RBUTTON */
	0,	/* 0x03 VK_CANCEL */
	0,	/* 0x04 VK_MBUTTON */
	0,	/* 0x05 */
	0,	/* 0x06 */
	0,	/* 0x07 */
	'\b',	/* 0x08 VK_BACK */
	'\t',	/* 0x09 VK_TAB */
	0,	/* 0x0a */
	0,	/* 0x0b */
	0,	/* 0x0c VK_CLEAR */
	'\r',	/* 0x0d VK_RETURN */
	0,	/* 0x0e */
	0,	/* 0x0f */

	0,	/* 0x10 VK_SHIFT */
	0,	/* 0x11 VK_CONTROL */
	0,	/* 0x12 VK_MENU */
	0,	/* 0x13 VK_PAUSE */
	0,	/* 0x14 VK_CAPITAL */
	0,	/* 0x15 */
	0,	/* 0x16 */
	0,	/* 0x17 */
	0,	/* 0x18 */
	0,	/* 0x19 */
	0,	/* 0x1a */
	'\033',	/* 0x1b VK_ESCAPE */
	0,	/* 0x1c */
	0,	/* 0x1d */
	0,	/* 0x1e */
	0,	/* 0x1f */

	' ',	/* 0x20 VK_SPACE */
	0,	/* 0x21 VK_PRIOR */
	0,	/* 0x22 VK_NEXT */
	0,	/* 0x23 VK_END */
	0,	/* 0x24 VK_HOME */
	'h',	/* 0x25 VK_LEFT */
	'k',	/* 0x26 VK_UP */
	'l',	/* 0x27 VK_RIGHT */
	'j',	/* 0x28 VK_DOWN */
	0,	/* 0x29 VK_SELECT */
	0,	/* 0x2a VK_PRINT */
	0,	/* 0x2b VK_EXECUTE */
	0,	/* 0x2c VK_SNAPSHOT or VK_COPY */
	0,	/* 0x2d VK_INSERT */
	0,	/* 0x2e VK_DELETE */
	0,	/* 0x2f VK_HELP */

	0,	/* 0x30 '0' */
	0,	/* 0x31 '1' */
	0,	/* 0x32 '2' */
	0,	/* 0x33 '3' */
	0,	/* 0x34 '4' */
	0,	/* 0x35 '5' */
	0,	/* 0x36 '6' */
	0,	/* 0x37 '7' */
	0,	/* 0x38 '8' */
	0,	/* 0x39 '9' */
	0,	/* 0x3a */
	0,	/* 0x3b */
	0,	/* 0x3c */
	0,	/* 0x3d */
	0,	/* 0x3e */
	0,	/* 0x3f */

	0,	/* 0x40 */
	0,	/* 0x41 'A' */
	0,	/* 0x42 'B' */
	0,	/* 0x43 'C' */
	0,	/* 0x44 'D' */
	0,	/* 0x45 'E' */
	0,	/* 0x46 'F' */
	0,	/* 0x47 'G' */
	0,	/* 0x48 'H' */
	0,	/* 0x49 'I' */
	0,	/* 0x4a 'J' */
	0,	/* 0x4b 'K' */
	0,	/* 0x4c 'L' */
	0,	/* 0x4d 'M' */
	0,	/* 0x4e 'N' */
	0,	/* 0x4f 'O' */

	0,	/* 0x50 'P' */
	0,	/* 0x51 'Q' */
	0,	/* 0x52 'R' */
	0,	/* 0x53 'S' */
	0,	/* 0x54 'T' */
	0,	/* 0x55 'U' */
	0,	/* 0x56 'V' */
	0,	/* 0x57 'W' */
	0,	/* 0x58 'X' */
	0,	/* 0x59 'Y' */
	0,	/* 0x5a 'Z' */
	0,	/* 0x5b */
	0,	/* 0x5c */
	0,	/* 0x5d */
	0,	/* 0x5e */
	0,	/* 0x5f */

	'0',	/* 0x60 VK_NUMPAD0 */
	'1',	/* 0x61 VK_NUMPAD1 */
	'2',	/* 0x62 VK_NUMPAD2 */
	'3',	/* 0x63 VK_NUMPAD3 */
	'4',	/* 0x64 VK_NUMPAD4 */
	'5',	/* 0x65 VK_NUMPAD5 */
	'6',	/* 0x66 VK_NUMPAD6 */
	'7',	/* 0x67 VK_NUMPAD7 */
	'8',	/* 0x68 VK_NUMPAD8 */
	'9',	/* 0x69 VK_NUMPAD9 */
	'*',	/* 0x6a VK_MULTIPLY */
	'+',	/* 0x6b VK_ADD */
	0,	/* 0x6c VK_SEPARATOR */
	'-',	/* 0x6d VK_SUBTRACT */
	'.',	/* 0x6e VK_DECIMAL */
	'/',	/* 0x6f VK_DIVIDE */

	0,	/* 0x70 VK_F1 */
	0,	/* 0x71 VK_F2 */
	0,	/* 0x72 VK_F3 */
	0,	/* 0x73 VK_F4 */
	0,	/* 0x74 VK_F5 */
	0,	/* 0x75 VK_F6 */
	0,	/* 0x76 VK_F7 */
	0,	/* 0x77 VK_F8 */
	0,	/* 0x78 VK_F9 */
	0,	/* 0x79 VK_F10 */
	0,	/* 0x7a VK_F11 */
	0,	/* 0x7b VK_F12 */
	0,	/* 0x7c VK_F13 */
	0,	/* 0x7d VK_F14 */
	0,	/* 0x7e VK_F15 */
	0,	/* 0x7f VK_F16 */

	0,	/* 0x80 VK_F17 */
	0,	/* 0x81 VK_F18 */
	0,	/* 0x82 VK_F19 */
	0,	/* 0x83 VK_F20 */
	0,	/* 0x84 VK_F21 */
	0,	/* 0x85 VK_F22 */
	0,	/* 0x86 VK_F23 */
	0,	/* 0x87 VK_F24 */
	0,	/* 0x88 */
	0,	/* 0x89 */
	0,	/* 0x8a */
	0,	/* 0x8b */
	0,	/* 0x8c */
	0,	/* 0x8d */
	0,	/* 0x8e */
	0,	/* 0x8f */

	0,	/* 0x90 VK_NUMLOCK */
	0,	/* 0x91 VK_SCROLL */
	0,	/* 0x92 */
	0,	/* 0x93 */
	0,	/* 0x94 */
	0,	/* 0x95 */
	0,	/* 0x96 */
	0,	/* 0x97 */
	0,	/* 0x98 */
	0,	/* 0x99 */
	0,	/* 0x9a */
	0,	/* 0x9b */
	0,	/* 0x9c */
	0,	/* 0x9d */
	0,	/* 0x9e */
	0	/* 0x9f */
};

int
virtkey(KEY_EVENT_RECORD keyrec)
{
	int vksize = sizeof(vktable) / sizeof(struct vkinfo);
	int vk, v;

	if ( ! keyrec.bKeyDown )
		return EOF;

	if ( keyrec.uChar.AsciiChar != 0 )
		return keyrec.uChar.AsciiChar;

	vk = keyrec.wVirtualKeyCode;
	if ( vk < 0 || vk >= vksize )
		return EOF;

	v = vktable[vk].val;
	if ( v )
		return v;
	return EOF;
}
