.\"========== Redefine NH to avoid widowing
.rn NH Nh
.nr nH 0
.de NH
.br
.if \\n(nH=\\$1 .sp 0.3i
.nr nH 1
.ne 1i
.Nh \\$1 \\$2
..
.\"========== Example macros
.de Ex
.br
.ne 0.75i
.IP "\fB\\$1\fP\ \ \ " \\$2
..
.de Ey
.sp -0.3v
.IP "\fB\\$1\fP\ \ \ " \\$2
..
.\"========== Put string in boldface & surround with quotes
.de qB
\%\*Q\fB\\$1\fP\*U\\$2
..
.\"========== 11 on 13 looks so much better than 10 on 12
.nr PS 11
.nr VS 13
.ps 11
.vs 13p
.nr PI 0.5i
.nr HM 0.9i
.nr FM 0.9i
.if n .nr PO 0.5i
.if n .nr LL 6.5i
.\"========== Turn hyphenation off, and make sure it stays off
.nh
.rm hy
.\"========== Headers in italics helps them to stand out from the text
.OH '\fIXvi Source Code Notes\fP''\fI%\fP'
.EH '\fI%\fP''\fIXvi Source Code Notes\fP'
.OF '\fI25th September 1992\fP''\fIPage %\fP'
.EF '\fIPage %\fP''\fI25th September 1992\fP'
.\"===========================================================================
.TL
Notes on the Xvi Source Code
.AU
Chris Downey
John Downey
.AB no
\fBXvi\fP (pronounced \fIecks-vee-eye\fP)
is a free, portable, multi-window implementation of the popular
.UX
editor \fBvi\fP.
.LP
This document contains information on how
to port \fBxvi\fP to systems not currently supported.
It also explains
how the \fBxvi\fP source code is arranged into modules,
and explains some of the data structures which are used,
so that modifications may be made if and when necessary
to the editor itself.
.AE
.\"===========================================================================
.NH 1
INTRODUCTION
.LP
\fBXvi\fP is intended to be portable to just about any system.
This is one of the central reasons for its existence; the
authors wish to be able to use the same editor everywhere.
.LP
The main body of the editor is (supposedly) fully portable,
relying only on standard facilities defined by the White
Book, and on a set of \fIprimitives\fP which are provided by a set
of one or more modules for each operating system.
If
.B __STDC__
is defined, certain ANSI C facilities will be used,
but the editor will compile with non-ANSI compilers.
.LP
Therefore, in order to port \fBxvi\fP to a new system, all that is
necessary is to provide the defined set of \fIprimitives\fP, and
then build the editor.
Or at least, that's the idea; we have refined the set of primitives
as we port the editor to new environments, and it's getting pretty easy now.
.LP
The rest of this document is divided into sections as follows:
.IP "\fISection 2: System-Specific Modules\fP"
This section deals with the layout of source files and makefiles
which you will have to deal with when porting \fBxvi\fP.
.IP "\fISection 3: Primitives Provided by xvi\fP"
Discusses what primitives are provided by the main body of the editor source
code for use by the system interface code.
.IP "\fISection 4: System Interface\fP"
Explains the primitives which need to be provided
in order to make \fBxvi\fP work.
.IP "\fISection 5: Data Structures\fP"
Details the internal data types used in the editor,
and any functions available for operating on those types.
.IP "\fISection 6: Source Files\fP"
Lists the source files comprising the editor,
and explains what functionality is provided by each one.
.\"===========================================================================
.NH 1
SYSTEM-SPECIFIC MODULES
.LP
The system-specific code normally consists of three (or more) files;
a
.qB .c
file, a
.qB .h
file, and a makefile.
For example:
.DS
.B
qnx.c
qnx.h
makefile.qnx
.R
.DE
comprise the system-specific module for the QNX operating system.
.LP
In most cases, the system-specific code is divided into two
or more modules, where one (called the \fIsystem interface module\fP)
is concerned with general interactions with the operating system
and the other (called the \fIterminal interface module\fP)
is designed for a specific interface to a display and keyboard
(and possibly, a mouse).
.LP
For example, the generic
.UX
implementation has
.B unix.c
and
.B unix.h
for the system interface module, and
.B termcap.c
and
.B termcap.h
for the terminal interface module; this should
work reasonably with any full-duplex terminal that can be
described in the
.B termcap
database.
On consoles with memory-mapped displays, or systems with graphic user
interfaces, however, it may be possible to achieve faster
display updating, and perhaps other benefits, by replacing
the
.B termcap
module with another one that makes better use of
whatever facilities are available.
For instance, there is an experimental version for SunView,
which allows mouse input on Sun workstations running the SunView window system.
.LP
On the other hand, the
.B termcap -specific
routines might
conceivably be useful on some other operating systems (such
as VMS), so in general it seemed a good idea to make the
.B termcap -specific
routines a separate module.
.LP
The current \%MS-DOS implementation has a separate terminal
interface module, which is designed specifically for IBM PC
compatible computers.
This is in the files
.DS
.B
ibmpc_a.asm
ibmpc_c.c
ibmpc.h
.R
.DE
The first of these is written in assembly language because
there are not enough routines common to the various \%MS-DOS
C compilers which reliably access the display and keyboard at
a low enough level.
.LP
The hardware-independent system interface module for \%MS-DOS is in
.DS
.B
msdos_a.asm
msdos_c.c
msdos.h
.R
.DE
The first of these is written in assembly
language for the same reason as is
.B ibmpc_a.asm .
.LP
Theoretically, different terminal interface modules could be
written for \%MS-DOS systems running on hardware which is not
IBM-compatible but, unfortunately, such systems seem to be
virtually extinct nowadays.
.LP
Sometimes more than one makefile is provided, as in the case of
.UX ,
where different versions work in slightly different ways.
.LP
It is, of couse, not necessary to provide all \(em or any \(em
of these files for a particular implementation; this is just a convention.
The makefile(s) for each system determine what
files are used in the compilation of the editor.
.LP
The following porting modules are available at present:
.\" ----------------------------------------------------------
.\" Note: this table does not fit very well with nroff output,
.\" so please try to avoid widening it if you add anything.
.TS
center, box;
c|c|c
l|l|l.
System	Makefile	Source Files
_
.sp 0.5v
UNIX
  BSD	\fBmakefile.bsd\fP	\fBunix.[ch] termcap.[ch]\fP
  System V \fB\(dg\fP	\fBmakefile.usg\fP	\fBunix.[ch] termcap.[ch]\fP
  AIX	\fBmakefile.aix\fP	\fBunix.[ch] termcap.[ch]\fP
  ULTRIX	\fBmakefile.ult\fP	\fBunix.[ch] termcap.[ch]\fP
  Xenix \fB\(dg\fP	\fBmakefile.xen\fP	\fBunix.[ch] termcap.[ch]\fP
  POSIX (e.g. BSDI)	\fBmakefile.pos\fP	\fBunix.[ch] termcap.[ch]\fP
  SunOS	\fBmakefile.sun\fP	\fBunix.[ch] termcap.[ch]\fP
  SunView	\fBmakefile.sv\fP	\fBunix.[ch] sunview.h\fP
		\fBsunfront.c sunback.c\fP
		\fBxvi.icn\fP
.sp 0.5v
_
.sp 0.5v
\%MS-DOS		\fBmsdos_c.c msdos.h\fP
		\fBibmpc_c.c ibmpc.h\fP
.sp 0.5v
  Microsoft C 5.*	\fBmakefile.msc\fP	\fB8086mm.inc ibmpc_a.asm\fP
  & MASM 5.*		\fBmsdos_a.asm\fP
.sp 0.5v
  Microsoft Quick C	\fBmakefile.qc\fP	\fB8086mm.inc ibmpc_a.asm\fP
  & MASM 5.*		\fBmsdos_a.asm\fP
.sp 0.5v
  Zortech C++ 2.*	\fBmakefile.zc2\fP	\fB8086mm.inc ibmpc_a.asm\fP
  & MASM 5.*		\fBmsdos_a.asm\fP
.sp 0.5v
  Zortech C++ 3.*	\fBmakefile.zc3\fP	\fB8086mm.inc ibmpc_a.asm\fP
  & MASM 5.*		\fBmsdos_a.asm\fP
.sp 0.5v
  Zortech C++ 3.*
  386 protected mode	\fBmakefile.386\fP	\fBpc386.[ch]\fP
.sp 0.5v
_
.sp 0.5v
OS/2 \fB\(dg\fP
  Version 1, text mode
  Microsoft C 5.1	\fBmakefile.os2\fP	\fBos2vio.[ch]\fP
  & MASM 5.1		\fBi286.asm\fP
.sp 0.5v
_
.sp 0.5v
QNX
  Version 2/3 (CII)	\fBmakefile.qnx\fP	\fBqnx.[ch]\fP
  Version 4 (Watcom C)	\fBmakefile.qn4\fP	\fBunix.[ch] termcap.[ch]\fP
.sp 0.5v
_
.sp 0.5v
TOS \fB\(dg\fP
  Lattice C	\fBmakefile.tos\fP	\fBtos.[ch] tos.lnk\fP
.sp 0.5v
.TE
.IP \fB\(dg\fP 2
Versions marked with
.B \(dg
probably do not work, as systems
have not been recently available to the authors for testing.
.\"===========================================================================
.NH 1
PRIMITIVES PROVIDED BY XVI
.NH 2
General Definitions
.LP
The file
.B xvi.h
should be included by all system-specific modules;
this file should also be edited
so that a system-specific header
file (or files), as determined by a predefined keyword,
will be included.
.LP
For instance, under
.UX ,
the word
.B UNIX
is defined by passing the
.B -DUNIX
flag to the C compiler
from the makefile, and
.B xvi.h
contains the following lines:
.DS
.B
#ifdef UNIX
#   include "unix.h"
#endif
.R
.DE
in order to obtain the
.UX -related
definitions from that header file.
.LP
Among the definitions in
.B xvi.h
are the following:
.Ex bool_t
A Boolean type having values
.B TRUE
or
.B FALSE .
.Ex const
.Ey volatile
These are defined out when
.B __STDC__
is not defined,
so that it is always safe to use them.
.LP
.B xvi.h
also includes various other header files which are needed.
The following system header files are always included:
.DS
.B
stdio.h
ctype.h
signal.h
string.h
.R
.DE
These files are included if
.B __STDC__
is defined:
.DS
.B
stddef.h
stdlib.h
limits.h
.R
.DE
and if
.B __STDC__
is not defined,
.B xvi.h
will provide its own
definitions for the following:
.DS
.B
INT_MAX
INT_MIN
ULONG_MAX

FILE	*fopen();
char	*malloc();
char	*getenv();
.R
.DE
Finally, one of the following header files will be included:
.DS
.B
stdarg.h
varargs.h
.R
.DE
depending on whether
.B __STDC__
is defined or not.
In order to make coding of
.B varargs
functions easier, a macro
.B VA_START ()
is defined, which takes the same arguments as the
ANSI-style
.B va_start (),
but which is also available in non-ANSI
environments (e.g. BSD).
.LP
In order to make it possible to use ANSI-style prototypes
for function declarations, but still allow compilation under
non-ANSI environments, the following macro is provided:
.DS
.B
#ifdef __STDC__
#   define  P(args) args
#else
#   define  P()     ()
#endif
.R
.DE
so that function declarations may be specified thus:
.DS
.B
extern FILE *fopen P((const char *, const char *));
.R
.DE
Please use this facility when you provide declarations for
your system primitives, unless your system always uses an
ANSI compiler.
.\"===========================================================================
.NH 2
Parameters
.LP
An important facility provided for use by system-specific
modules is access to the editor's parameter table.
This is achieved by means of some apparent functions,
and a set of
.B #define d
token values.
The functions are:
.Ex "void set_param(int n, val)"
This function sets the indicated parameter to the
passed value, which must be of an appropriate type.
Parameter values may be obtained by means of the following
functions (actually macros):
.Ex "char *Ps(int n)"
return value of string parameter
.Ex "int Pn(int n)"
return value of numeric parameter
.Ex "bool_t Pb(int n)"
return value of boolean parameter
.Ex "char **Pl(int n)"
return value of list parameter (a
\fBNULL\fP-terminated array of character pointers)
.Ex "int Pen(int n)"
return numeric value (index) of enumerated parameter
.Ex "char **Pes(int n)"
return string value of enumerated parameter
.LP
In all cases, the \fBint n\fP argument is the index of the parameter
in the table; a set of
.B #define s
is provided, of the form:
.DS
.B P_name
.DE
which map the parameter names into integral values.
Thus, for example, we might obtain the value of the
.B colour
parameter:
.DS
.B
colour = Pn(P_colour);
.R
.DE
or set the value of the
.B helpfile
parameter:
.DS
.B
set_param(P_helpfile, "/usr/lib/xvi/help");
.R
.DE
.\"===========================================================================
.NH 1
SYSTEM INTERFACE
.NH 2
Introduction
.LP
There follows a list of the primitives which must be provided
either by the system interface module or by the underlying OS.
Note that it is perfectly acceptable to implement functions or external
variables as
macros
so long as they \*Qlook the same\*U as the definitions
below.
As a guideline, anything which is (a) in capitals, or (b) is a
\fBconst\fP variable, will be implemented as a
.B #define
for most
systems.
.LP
When you want to actually do the port, it is highly
recommended that you copy the system-specific files for the
system which seems closest to your own, and modify those
files, rather than starting from scratch.
.LP
All the following symbols should be defined in the system
interface module, or by standard header files already included
by
.B xvi.h ,
or by other header files explicitly included by
the system-specific header file:
.\" ----- Standard items, not always available -----
.Ex "const unsigned int MAXPATHLEN"
The maximum number of characters in a pathname.
.Ex "const unsigned int MAXNAMLEN"
The maximum number of characters in a filename.
.Ex "int remove(char *filename)"
Remove the named file as per ANSI.
.Ex "int rename(char *old, char *new)"
Rename the file \fBold\fP to \fBnew\fP as per ANSI.
.Ex "void sleep(unsigned int seconds)"
Put the process to sleep for the given number of seconds.
.\" ----- xvi specials -----
.Ex "const char * const DIRSEPS"
The pathname separators supported for system calls (e.g.
\fB"\e\e\|/"\fP
for \%MS-DOS).
.Ex "FILE *fopenrb(char *file)"
.Ey "FILE *fopenwb(char *file)"
Like the standard
.B fopen()
library call,
but they both open files in \*Qbinary\*U mode
(i.e. no conversion of cr/lf/crlf is done),
for reading and writing respectively.
.Ex "bool_t exists(char *filename)"
Returns
.B TRUE
if the named file exists.
.Ex "bool_t can_write(char *filename)"
Returns
.B TRUE
if the named file can be written,
i.e. if a \fBfopenwb(filename)\fP will succeed.
.Ex "char *fexpand(char *filename)"
Returns a filename-expanded version of the passed filename.
.Ex "#define SETVBUF_AVAIL"
.Ey "const unsigned int READBUFSIZ"
.Ey "const unsigned int WRTBUFSIZ"
If
.B SETVBUF_AVAIL
(or
.B __STDC__ )
is defined, these constant values
are used to set I/O buffer sizes (using the \fBsetvbuf()\fP function)
for reading and writing files.
Note that if buffers of these sizes are unavailable at runtime,
the editor will try to allocate smaller buffers by iteratively
halving the buffer size until the allocation succeeds.
It is therefore acceptable for these values to be quite large.
.Ex "char *tempfname(const char *filename)"
Create a unique name for a temporary file,
possibly using \fBfilename\fP as a base
(this will be used by
.B do_preserve()
to create a backup file
for the file named by
.B filename ).
The string returned must have been allocated using
.B malloc() ;
.B NULL
can be returned if there is no more memory available.
.Ex "int call_system(char *command)"
Invoke the given command in a subshell.
This is used for shell escapes from \fBxvi\fP.
The command string may contain metacharacters
which are expected to be expanded
by a command interpreter, e.g.
.UX
.B /bin/sh ,
\%MS-DOS
.B command.com .
Return value is 0 for success.
In many environments, this call may safely be
.B #define d
as
.B system(command) .
.Ex "int call_shell(char *shell)"
Invoke the named shell.
This is used for the
.B :shell
command.
It may be mapped into
.B call_system() ,
but is separate on some systems for
efficiency reasons (i.e. not invoking two shells to get one).
Return value is 0 for success.
.Ex "bool_t"
.Ey "sys_pipe(char *cmd, int (*wf)(FILE *), long (*rf)(FILE *))"
Used for the
.B !
command.
The first parameter is the command to invoke, while the second and third
are functions which should be called with an open file pointer in order
to
write out old,
or read in new
lines (respectively).
Note that if \*Qreal\*U pipes are not available, it is acceptable
to implement this function using temporary files, but the \fBwf\fP
function must obviously be called before \fBrf\fP.
.Ex "void sys_exit(int code)"
Exit with given exit status.
This routine must not return.
The editor is considered \*Qdead\*U once it has been called, and no
further calls to editor functions should be made.
.Ex "void delay(void)"
Delay for a short time, about a fifth of a second.
This is used for showing matching brackets when \fBshowmatch\fP is set.
It is acceptable to just return if implementing this is not easy.
.\"===========================================================================
.NH 2
Screen Control
.LP
An instance of the following structure must be defined
in order to allow screen output to take place:
.DS L
.ta 1.3i 3i
.B
  typedef struct virtscr {
    genptr	*pv_window;
    int	pv_rows;
    int	pv_cols;
/* public: */
    VirtScr	*(*v_new)(VirtScr *);
    void	(*v_close)(VirtScr *);

    int	(*v_rows)(VirtScr *);
    int	(*v_cols)(VirtScr *);

    void	(*v_clear_all)(VirtScr *);
    void	(*v_clear_line)(VirtScr *);

    void	(*v_goto)(VirtScr *, int row, int col);
    void	(*v_advise)(VirtScr *, int row, int col,
		int index, char *str);

    void	(*v_write)(VirtScr *, int row, int col, char *str);
    void	(*v_putc)(VirtScr *, int row, int col, int ch);

    void	(*v_set_colour)(VirtScr *, int colour);
    int	(*v_colour_cost)(VirtScr *);

    void	(*v_flush)(VirtScr *);

    void	(*v_beep)(VirtScr *);

/* optional: not used if NULL */
    void	(*v_insert)(VirtScr *, int row, int col, char *str);

    int	(*v_scroll)(VirtScr *, int start, int end, int nlines);
  } VirtScr;
.R
.DE
.LP
The first three fields in this structure are \*Qprivate\*U, for use only
within the implementation of the \*Qpublic\*U functions.
The remaining fields are all function pointers, and are described below.
Note that all functions have at least one parameter, which is a pointer
to the instance of the \fBVirtScr\fP in question.
This is always referred to as \fBvs\fP below.
Note also that the top-left-hand corner of the window is taken to be (0,0).
.Ex "v_new(vs)"
Obtain a new \fBVirtScr\fP, and return a pointer to it.
This is not used at present, and should return
.B NULL .
.Ex "v_close(vs)"
Close the window to which \fBvs\fP refers.
.Ex "v_rows(vs)"
Return the number of rows in \fBvs\fP.
.Ex "v_cols(vs)"
Return the number of columns in \fBvs\fP.
.Ex "v_clear_all(vs)"
Clear the window completely.
.Ex "v_clear_line(vs, int row, int col)"
Clear the specified line, from the given column to the right hand edge
of the window, inclusive.
.Ex "v_goto(vs, int row, int col)"
Move the cursor to the specified row and column.
.Ex "v_advise(vs, int row, int col, int index, char *str)"
This function is called when the editor is about to produce some
output on the same line as the last output, but separate from it
by one or more characters.
The destination position is the coordinate pair \fB(row, col + index)\fP,
and \fBstr\fP contains the string of characters which are in the window
starting at position \fB(row, col)\fP.
Where there is a cost incurred
by moving the cursor to a specific screen position,
the terminal interface module may decide to write the intervening characters
to the screen rather than using a specific \*Qmove cursor\*U sequence,
in order to minimise the number of characters written to the terminal.
.Ex
Note that for many environments, the cost of re-positioning the cursor is
nil, and under these circumstances this function need not do anything.
.Ex "v_write(vs, int row, int col, char *str)"
Write the specified string of characters into the window, starting at
the specified row and column.
The parameters will be such that the string will always fit into
a single line of the window, i.e. no line-wrapping is necessary;
however, it is quite possible for the string to end on the last character
of a line, and some implementations will need to take special
precautions to handle this correctly.
.Ex "v_putc(vs, int row, int col, int ch)"
This is like \fBv_write\fP but for a single character.
.Ex "v_set_colour(vs, int colour)"
Set the colour for all subsequent output (including clearing of
lines or the whole window) to the specified colour.
The meaning of the value is system-specific.
.Ex "v_colour_cost(vs)"
Return the number of extra characters which are taken up in the window
by a colour change.
This is almost always 0,
but there exist some terminals for which it is not
(see the
.qB sg
.B termcap
capability).
.Ex "v_flush(vs)"
Flush all screen output, and move the cursor on the screen to the correct
position.
The screen need not actually be updated until either this function is called,
or \fBxvi_handle_event()\fP returns.
.Ex "v_beep(vs)"
Beep.
It is acceptable to flash the screen or window if no audio facility
is available.
.Ex "v_insert(vs, int row, int col, char *str)"
This function inserts the given string at the given position,
pushing any other characters on the same row to the right.
If such a facility is not available,
the function pointer should be set to
.B NULL .
.Ex "v_scroll(vs, int start, int end, int nlines)"
This function scrolls the set of lines between \fBstart\fP and \fBend\fP
(inclusive) by \fBnlines\fP lines.
If \fBnlines\fP is positive, \fInormal\fP scrolling should be done,
i.e. the lines should be moved upwards with respect to the window.
If \fBnlines\fP is negative, scrolling should be in the reverse direction.
The lines which are left by the scrolling should be cleared.
The function should return non-zero if the scrolling was successful,
otherwise 0.
.Ex
If scrolling is not available, the function pointer should be set to
.B NULL .
.\"===========================================================================
.NH 2
Parameters
.LP
Default values should be
.B #define d
for certain parameters
as follows:
.TS
center, box;
c|c|c
l|c|l.
Parameter Name	Type	\fB#define\fP name
_
\fBsyscolour\fP	numeric	\fBDEF_SYSCOLOUR\fP
\fBcolour\fP	numeric	\fBDEF_COLOUR\fP
\fBstatuscolour\fP	numeric	\fBDEF_STCOLOUR\fP
\fBroscolour\fP	numeric	\fBDEF_ROSCOLOUR\fP
\fBhelpfile\fP	string	\fBHELPFILE\fP
\fBformat\fP	string	\fBDEF_TFF\fP
.TE
.\===========================================================================
.NH 2
File Formats
.LP
The functions in \fBxvi\fP which read and write text
files are aware of several different newline conventions
(for example,
\fB"\e\^n"\fP on
.UX ,
\fB"\e\^r\^\e\^n"\fP on \%MS-DOS, and so on), so
that any version of the editor can read and write any of the
supported formats.
The value of the \fBformat\fP parameter
(which can be set to
.qB unix ,
.qB msdos ,
.qB macintosh ,
etc.)
determines which format is currently being used.
If you are porting \fBxvi\fP to a system with a newline convention which
isn't one of those currently supported (see the table called
.B tftable
in
.B fileio.c )
you may have to add a new entry to the table.
.LP
Unfortunately, the current design is not as general as it ought to be.
If you happen to be porting to VMS,
or some other system which doesn't use either
a single character
or a consecutive pair of characters
to represent a newline,
you will have quite a lot of work to do
if you want to retain the facility for
converting between file formats within the editor.
.LP
In any case, your system interface module should define
.B DEF_TFF
to be the index of the entry in \fBtftable\fP
which represents the default format for your system.
This is the value for
.B Pen(P_format)
which will be
compiled into the parameter table.
.\"===========================================================================
.NH 2
Notes on Termcap Implementation
.LP
There exists a \fBtermcap\fP implementation of the terminal interface,
currently only used for the
.UX
port.
This module could quite easily be re-used for other systems if desired;
the following routines would need to be defined by the system module:
.Ex "void foutch(int c)"
Output a single character to the terminal.
This must be implemented as a function, not a macro, because it is passed
as a parameter into the
.B termcap
library.
.Ex "void moutch(int c)"
Same as
.B foutch()
except that it can be implemented as a macro.
This will be used by the
.B termcap
interface module to write characters to
the screen.
.Ex "void oflush(void)"
Flush buffered output to the terminal.
.\"===========================================================================
.NH 2
Entering/Leaving Visual Mode
.LP
Some facility is commonly necessary for the system interface module
to be able to tell the terminal interface module to enter or exit
\fIvisual\fP mode.
This might mean changing the terminal state between \*Qraw\*U and \*Qcooked\*U
modes, or switching display pages.
No specific interface for this is defined,
although the standard
.UX
and \%MS-DOS implementations do use such a facility,
and the interface functions for both systems are identically defined.
.\"===========================================================================
.NH 2
Function Keys\|\|/\|\|Mouse Handling
.LP
Function key values are coded into a set of
.B #define d
constants in the file
.B ascii.h ;
e.g. the value
.B K_UARROW
might be given as input when the keyboard up-arrow key has been pressed.
.LP
If the global variable
.B State
is not equal to
.B NORMAL ,
all
function keys except for a backspace key are invalid input.
If an invalid key is pressed, the safest strategy may be to
beep and wait for another key to be pressed.
.B NORMAL
is defined in
.B xvi.h .
.LP
Another facility which may be provided
is handling mouse input on systems where it is available.
The strategy for interpreting mouse input is controlled
by the
.B mouseclick()
function (in
.B mouse.c );
the idea is
to make the strategy independent of any specific device interface.
If a mouse button is pressed before a keyboard key is pressed,
the following routine should be called:
.DS
.B "mouseclick(int row, int column);"
.DE
where row and column are the current co-ordinates, counted
in character positions, of the mouse pointer within the
screen or editing window.
If the mouse is moved while a button is held down, the routine
.DS
.B "mousedrag(int startrow, int endrow, int startcolumn, int endcolumn);"
.DE
should be called with co-ordinates describing the movement.
If the global variable
.B State
is not equal to
.B NORMAL ,
mouse input can be ignored altogether.
.LP
All this will be considerably tidied up at a later stage, when we have
proper
.B xvEvent
types for function keys and mouse actions.
.\"===========================================================================
.NH 2
Main
.LP
Finally, the system interface module must provide a \fBmain()\fP function.
This function must call \fBxvi_startup(vs, argc, argv, env)\fP at startup,
with parameters as follows:
.Ex "VirstScr *vs;"
This is a pointer to the \fBVirtScr\fP structure for the first window,
or for the terminal screen.
.Ex "int argc, char **argv;"
These are as for a \fBmain()\fP function.
.Ex "char *env;"
This is an environment string, normally the return value from
\fBgetenv("XVINIT")\fP.
If the concept of environment variables does not exist,
a string of the form \fB"source\ \fIfilename\fB"\fR may be passed instead,
so as to allow users to localise their usage of the editor.
.LP
The return value from \fBxvi_startup()\fP is a pointer, which will
be used in future to identify the window for input events.
For now, it should be stored in the \fBVirtScr\fP's \fBpv_window\fP field.
.LP
Having called \fBxvi_startup()\fP, input events may then be passed
to the editor by calling \fBxvi_handle_event\fP with a pointer to
an \fBxvEvent\fP structure as the sole argument.
This structure is defined as follows:
.DS
.B
typedef struct event {
    enum {
        Ev_char,
        Ev_timeout
    }                   ev_type;
    union {
        /* Ev_char: */
        int evu_inchar;

        /* Ev_timeout: */
    }                   ev_u;
} xvEvent;

#define	ev_inchar	ev_u.evu_inchar
.R
.DE
.LP
The \fBev_type\fP field is a tag which identifies the type of event
which has occurred.
At present, only two events are supported: an input character from
the user, and a timeout.
The union which follows contains data associated with each event type;
currently only the type \fBEv_char\fP requires data, as may be seen.
The
.B #define
for \fBev_inchar\fP is provided purely for convenience.
.LP
The return value from \fBxvi_handle_event()\fP is a long integer value
which is the time in milliseconds for which the editor is prepared
to wait for more input.
If no input arrives within that time, the function should be called
again with an event of type \fBEv_timeout\fP.
The timeout value returned may be 0L, indicating that no timeout is necessary.
It is very important that timeouts should actually be implemented because
they are needed for the
.B preserve
facility.
.LP
Currently, if a keyboard interrupt is received,
.B xvi_handle_event()
need not be called
(it should,
in any case,
never be called from an asynchronous interrupt
or signal
handler)
but the global variable
.B kbdintr
should be set to a non-zero value.
.\"===========================================================================
.NH 1
DATA STRUCTURES
.LP
Structures used in \fBxvi\fP are all typedef'd,
and all begin with a capital letter.
They are defined in
.B xvi.h .
The following data structures are defined:
.NH 2
Line
.LP
This structure is used to hold a single text line.
It contains forward and backward pointers which are connected together
to form a two-way linked list.
It also contains a pointer to an allocated text buffer,
an integer recording the number of bytes allocated for the text,
and the line number (an unsigned long).
The text is null-terminated, and the space allocated for it may be
grown but is never shrunk.
The maximum size of this space is given by
.B MAX_LINE_LENGTH .
.LP
The line number is used when showing line numbers on screen, but this
is secondary to its main purpose of providing an ordering on lines;
the ordering of two lines in a list may be established by simply
comparing their line numbers
(macros are available for this purpose; see later for details).
.NH 2
Buffer
.LP
This structure holds the internal representation of a file.
It contains pointers to the
linked list of lines which comprise the actual text.
We always allocate an extra line at the beginning and the end,
with line numbers 0 and
.B MAX_LINENO
respectively,
in order to make the code which deals with this structure easier.
The line numbers of \fBLine\fP structures in a \fBBuffer\fP
are always maintained by code in \fBundo.c\fP,
which is the only module which ever changes the text of a \fBBuffer\fP.
.LP
The \fBBuffer\fP structure also contains:
.IP \(bu
flags, including readonly and modified
.IP \(bu
current filename associated with the buffer
.IP \(bu
temporary filename for buffer preservation
.IP \(bu
space for the
.B mark
module to store information about marked lines
.IP \(bu
space for the
.B undo
module to store information about the last change
.IP \(bu
number of windows associated with the buffer
.LP
The following macros are used to find out certain information
about \fBLine\fPs within \fBBuffers\fP:
.Ex "lineno(Buffer *b, Line *l)"
Returns the line number of the specified \fBLine\fP,
which belongs to the specified \fBBuffer\fP.
.Ex "earlier(Line *l1, Line *l2)"
Returns
.B TRUE
if \fBl1\fP is earlier in the buffer than \fBl2\fP.
.Ex "later(Line *l1, Line *l2)"
Returns
.B TRUE
if \fBl1\fP is later in the buffer than \fBl2\fP.
.Ex "is_lastline(Line *l1)"
Returns
.B TRUE
if \fBl1\fP is the last line (i.e. the extra line
at the end, not the last text line) of the buffer.
.Ex "is_line0(Line *l1)"
Returns
.B TRUE
if \fBl1\fP is the 0th line (i.e. the extra line
at the start, not the first text line) of the buffer.
.NH 2
Posn
.LP
This structure is very simple; it contains a \fBLine\fP pointer and an integer
index into the line's text, and is used to record a position within a buffer,
e.g. the current cursor position.
.LP
These functions are available for operating on \fBPosn\fP structures:
.Ex "gchar(Posn *)"
Returns the character which is at the given position.
.Ex "inc(Posn *)"
Increments the given position, moving past
end-of-line to the next line if necessary.
The following type is returned:
.DS L
.B
.ta 2i
  enum mvtype {
      mv_NOMOVE,	/* at beginning or end of buffer */
      mv_SAMELINE,	/* still within same line */
      mv_CHLINE,	/* changed to different line */
      mv_EOL,	/* at terminating '\e0' */
  };
.R
.DE
.Ex "dec(Posn *)"
As for \fBinc()\fP but decrements the position.
.Ex "lt(Posn *p1, Posn *p2)"
Returns
.B TRUE
if the position specified by \fBp1\fP is earlier in the buffer
than that specified by \fBp2\fP.
.NH 2
Xviwin
.LP
This structure maps a screen window onto a \fBBuffer\fP.
It contains:
.IP \(bu
a pointer to the \fBBuffer\fP structure which it is mapped onto
.IP \(bu
the cursor's \fIlogical\fP position in the buffer (a \fBPosn\fP structure)
.IP \(bu
the cursor's \fIphysical\fP position in the window (row and column)
.IP \(bu
information about size and location of screen window
.IP \(bu
current text of status line
.IP \(bu
forward and backward pointers to other windows
.LP
Note that there is at least one \fBXviwin\fP for every \fBBuffer\fP.
.LP
When the editor was modified to support buffer windows, many
global variables were moved into the \fBBuffer\fP and \fBXviwin\fP structures;
some were left as globals.
For instance, the
.I undo
and
.I mark
facilities are obviously buffer-related,
but
.I yank
is useful if it is global
(actually static within its own module);
it was decided that
.I search
and
.I redo
should also be global.
.LP
Some modules have their own internal static data structures;
for instance, the
.B search
module remembers the last pattern
searched for.
Also, certain modules use data structures which are included
in more global ones; e.g. each \fBBuffer\fP structure contains some
data used only within
.B undo.c .
This is not very well structured, but in practice it's quite
clean because we simply ensure that references to such structures
are kept local to the module which \*Qowns\*U them.
.NH 2
Mark
.LP
This data structure records a mark (defined by the \fBm\fP command).
It contains a \fBPosn\fP and a character field to hold the letter
which defines the mark.
Each \fBBuffer\fP contains an array of these structures for holding
alphabetic marks, plus one for the previous context mark
(as used by the
.B ''
and
.B ``
commands).
The file
.B mark.c
deals with marks.
.NH 2
Change
.LP
This structure records a single change which has been made to a buffer.
It also contains a pointer, so that it may be formed into a list.
See the discussion of
.B undo.c
below for further details.
.NH 2
Flexbuf
.LP
This structure is used to store text strings for which the length is unknown.
The following operations are defined for this type.
All functions take a Flexbuf pointer as a parameter.
.Ex "flexnew(f)"
Initialise a Flexbuf; not needed for static Flexbufs.
.Ex "flexclear(f)"
Truncate a Flexbuf to zero length, but don't free its storage.
.Ex "flexdelete(f)"
Free all storage belonging to a Flexbuf.
.Ex "flexempty(f)"
Return
.B TRUE
if the Flexbuf is empty.
.Ex "flexlen(f)"
Return the number of characters in the Flexbuf.
.Ex "flexrmchar(f)"
Remove the last character from a Flexbuf.
.Ex "flexpopch(f)"
Remove the first character from a Flexbuf and return it.
.Ex "flexgetstr(f)"
Return a pointer to the string contained in the Flexbuf.
.Ex "flexaddch(f, c)"
Add the character \fBc\fP to the end of the Flexbuf.
.Ex "lformat(f, fmt, ...)"
A subset of \fBsprintf()\fP but for Flexbufs.
.Ex "vformat(f, fmt, va_list)"
A subset of \fBvsprintf()\fP but for Flexbufs.
.LP
The last two functions are especially useful, since they avoid
the usual problems with the lack of bounds-checking in \fBsprintf()\fP.
All code in the editor itself now uses Flexbufs to avoid the possibility
of buffer overruns, and to reduce the size of the executable.
Some OS-specific modules, however, may still use the \fBprintf()\fP family.
The subset of \fBprintf\fP-like format specifiers implemented includes
those for integers and strings, but not for floating-point numbers.
.NH 2
bool_t
.LP
A simple Boolean type; has values
.B TRUE
and
.B FALSE ,
which are defined as
1 and 0 so as to be compatible with C comparison operators.
.NH 2
xvEvent
.LP
This type is defined in the previous section,
since it forms part of the porting interface.
.NH 2
VirtScr
.LP
This type represents a virtual screen, and
is constructed in a similar way to a \fIclass\fP.
It contains some function pointers which may be used to manipulate the
screen in various ways, and some private data which is used by the
implementation of the class.
.LP
The old terminal interface, which consisted of a set of disparate functions,
is being replaced by the \fBVirtScr\fP interface;
the first step in this process has been accomplished
by the provision of a default \fBVirtScr\fP implementation
using the old primitive functions.
New, native, \fBVirtScr\fP implementations may now be coded,
which will increase the efficiency of screen output.
.LP
As the final stage, a windowing implementation of the \fBVirtScr\fP class
will be provided, using the underlying \fBVirtScr\fP implementations,
and the window-handling code in the editor will be modified to
that each occurrence of an \fBXviwin\fP references its own \fBVirtScr\fP.
It will then be possible to build a version of the editor which operates in a
true windowing environment by using a separate screen window for each buffer,
instead of the current vertical-split method.
.LP
A full definition of the \fBVirtScr\fP type will be found
in the previous section.
.\"===========================================================================
.NH 2
Global Variables
.LP
There are only a few global variables in the editor.
These are the important ones:
.Ex curbuf 0.7i
pointer to the current \fBBuffer\fP
.Ex curwin 0.7i
pointer to the current \fBXviwin\fP
.Ex State 0.7i
the current \fIstate\fP of the editor;
controls what we do with input characters.
The value is one of the following:
.RS
.Ex NORMAL 1i
The default state; \fBvi\fP-mode commands may be executed
.Ex INSERT 1i
Insert mode, i.e. characters typed get inserted into the current buffer
.Ex REPLACE 1i
Replace mode, characters in the buffer get overwritten by what is typed
.Ex CMDLINE 1i
Reading a colon-command, regular expression or pipe command
.Ex DISPLAY 1i
Displaying text, i.e. \fB:p\fP command, or \fB:set\fP or \fB:map\fP with no argument
.RE
.Ex echo 0.7i
This variable controls what output is currently displayable.
It is used at various points within the editor to stop certain
output which is either undesirable or sub-optimal.
It must always
be restored to its previous value after the code which changed it
has finished what it is doing.
.Ex kbdintr 0.7i
This can be set to a non-zero value to indicate that an asynchronous
user-generated interrupt (such as a keyboard interrupt) has occurred.
See the discussion of event handling in the previous section.
.\"===========================================================================
.nr PI 1i	\" Extra indentation for filenames
.NH 1
SOURCE FILES
.LP
The header file
.B xvi.h
contains all the type definitions
used within the editor, as well as function declarations etc.
.LP
The following source files form the primary interface to the editor:
.Ex startup.c
Entry point for the editor.
Deals with argument
and option parsing and initial setup, calling
module initialisation functions as necessary.
.Ex events.c
Contains the routine \fBxvi_handle_event()\fP, which is
the entry point for handling input to the editor;
input is passed to different routines according to the
\fBState\fP variable.
Timeouts on input are also handled here, by calling appropriate
routines in \fBmap.c\fP or \fBpreserve.c\fP.
.Ex edit.c
Deals with insert and replace modes.
.Ex normal.c
Handles normal-mode commands.
.Ex map.c
This file is responsible for all input mapping (both set up by the
\fB:map\fP command and internally for function-key mappings;
it also implements a stuff-characters-into-the-input-stream
function for use within the editor.
This is used, for example, to implement command redo
(but \fInot\fP to implement \*Qundo\*U and \*Qput\*U as in STEVIE).
.\"-----------------------------------------------------------------
.sp
.LP
Colon (\fBex\fP-type) commands are handled by this group:
.Ex cmdline.c
Decodes and executes colon commands.
.Ex ex_cmds1.c
File-, \fBBuffer\fP- and \fBXviwin\fP-related colon commands.
.Ex ex_cmds2.c
Other colon commands (e.g. shell escape).
.\"-----------------------------------------------------------------
.sp
.LP
Screen updating is done within the following files:
.Ex screen.c
Screen updating code, including handling of line-based entry
(for colon commands, searches etc) as they are typed in,
and display-mode stuff (for parameter displaying,
.B :g/re/p
etc).
.Ex cursor.c
This file contains the single function \fBcursupdate()\fP,
which is responsible for deciding where the physical screen cursor
should be, according to the position of the logical cursor in the
buffer and the position of the window onto that buffer.
This routine is not very optimal, and will probably disappear in due course.
.Ex defscr.c
This file contains the default implementation of the \fBVirtScr\fP class,
on top of the old terminal/system interface.
.Ex status.c
Functions to update the status line of a window; there are different
functions to display file information (name, position etc.)
and error/information messages.
.\"-----------------------------------------------------------------
.sp
.LP
These files deal with specific areas of functionality:
.Ex find.c
Search functions: all kinds of searches, including character-based
and word-based commands, sections, paragraphs, and the interface to
\*Qreal\*U searching (which is actually done in
.B search.c ).
.Ex mark.c
Provides primitives to record marks within a \fBBuffer\fP,
and to find the marks again.
.Ex mouse.c
Code to handle mice moving the cursor around and resizing windows.
.Ex param.[ch]
Code to handle setting of, and access to, parameters.
(These are things like \fBtabstops\fP, \fBautoindent\fP, etc.)
.Ex pipe.c
Handles piping through system commands.
.Ex preserve.c
File preservation routines.
.Ex search.c
Code for pattern-searching in a buffer, and for substitutions
and global execution.
Uses \fBregexp.[ch]\fP for the actual regular expression stuff.
.Ex tags.c
Routines to handle tags \(em for \fB:tag\fP, \fB-t\fP and \fB^]\fP.
.Ex undo.c
Code to deal with doing and undoing; i.e. making and unmaking
changes to a buffer.
This is one of the more complex and delicate files.
.Ex yankput.c
Code to deal with yanking and putting text, including named buffers.
.\"-----------------------------------------------------------------
.sp
.LP
while these files provide lower-level functions:
.Ex alloc.c
Memory allocation routines.
.Ex ascii.[ch]
Deals with the visual representation of special
characters on the display (e.g. tabs, control chars).
.Ex buffers.c
Routines dealing with the allocation and freeing of \fBBuffers\fP.
.Ex fileio.c
File I/O routines; reading, writing, re-editing files.
Also handling of the \fBformat\fP parameter.
.Ex flexbuf.c
Flexible-length character-buffer routines.
.Ex misccmds.c
Miscellaneous functions.
.Ex movement.c
Code to deal with moving the cursor around in the buffer,
and scrolling the screen etc.
.Ex ptrfunc.[ch]
Primitives to handle \fBPosn\fP structures; including various
operators to compare positions in a text buffer.
.Ex "regexp.[ch], regmagic.h"
Regular-expression stuff, originally written by Henry Spencer
(thanks Henry) and slightly hacked for use within \fBxvi\fP.
.Ex signal.c
Handling of terminal-generated signals in an ANSI environment.
.Ex virtscr.h
Virtual Screen interface definition.
This is a new part of \fBxvi\fP, and is not yet fully completed.
When it is finished, it will provide the ability to implement
\*Qnative\*U versions of \fBxvi\fP under various windowing systems,
in a clean and wholesome way.
Currently there is a single instance of the \fBVirtScr\fP class, which
is defined on top of the old system/terminal interface.
.Ex windows.c
Code to deal with creating, deleting, resizing windows.
.Ex version.c
Contains only the version string.
.\"-----------------------------------------------------------------
