.\"========== Macro definitions
.\"========== Three-column output
.de c3
.ta 1.5i 3i 4.5i
\\$1	\\$2	\\$3
.br
..
.\"========== Put string in boldface & surround with quotes
.de qB
\%\*Q\fB\\$1\fP\*U\\$2
..
.\"========== Redefine NH to avoid widowing
.rn NH Nh
.de NH
.if \\$1=1 .ne 1.5i
.ne 1i
.Nh \\$1 \\$2
..
.\"========== End of macros
.\"========== 11 on 13 looks so much better than 10 on 12
.nr PS 11
.nr VS 13
.ps 11
.vs 13p
.nr HM 0.9i
.nr FM 0.9i
.if n .nr PO 0.5i
.if n .nr LL 6.5i
.\"========== Turn hyphenation off, and make sure it stays off
.nh
.rm hy
.\"========== Headers in italics helps them to stand out from the text
.OH '\fISummary of Differences between Vi and Xvi\fP''\fI%\fP'
.EH '\fI%\fP''\fISummary of Differences between Vi and Xvi\fP'
.OF '\fI25th September 1992\fP''\fIPage %\fP'
.EF '\fIPage %\fP''\fI25th September 1992\fP'
.\"===================== End of header; start of document ====================
.TL
Summary of Differences between Vi and Xvi
.AU
Chris Downey
John Downey
.AB no
\fBXvi\fP (pronounced \fIecks-vee-eye\fP)
is a free, portable, multi-window implementation of the popular
.UX
editor \fBvi\fP.
.LP
This document summarises the differences between the ``real'' \fBvi\fP
and \fBxvi\fP.
These differences are divided into three groups:
\fIunimplemented features\fP,
\fIdifferences\fP
and \fIenhancements\fP,
and each of these is described in the following sections.
.AE
.\"===========================================================================
.NH 1
Unimplemented Features
.\"---------------------------------------------------------------------------
.KS
.NH 2
Ex Mode
.LP
The main area in which \fBxvi\fP is lacking is \fBvi\fP's
.B ex
mode,
which is not implemented at all (and neither are \fBedit\fP, \fBe\fP,
and \fBopen\fP modes).
However, many of the \fBex\fP commands are available in \fBxvi\fP as
colon commands;
the colon commands that have not been implemented are mostly those which offer
the same functionality as other commands in \fBvi\fP mode.
.KE
.KS
.LP
Specifically, the following \fBex\fP commands are not implemented,
and probably never will be within \fBxvi\fP:
.DS
.B
.c3 insert undo ex
.c3 change open |
.c3 append z
.R
.DE
.KE
.KS
.nh
.rm hy
while these \fBex\fP commands, although not currently implemented,
may be added as colon commands at some time in the future:
.DS
.B
.c3 abbreviate recover write>>
.c3 unabbreviate join
.R
.DE
.KE
.\"---------------------------------------------------------------------------
.NH 2
Vi commands
.LP
The following \fBvi\fP mode commands are not implemented,
although there is no real reason why they shouldn't be:
.DS
.B
.c3 U =
.R
.DE
.KS
while this one is inappropriate in the context of \fBxvi\fP,
since there is no
.B ex
mode:
.DS
.B Q
.DE
.KE
.\"---------------------------------------------------------------------------
.KS
.NH 2
Parameters
.LP
The following parameters have not been implemented,
and probably won't be:
.DS
.B
.c3 ada lisp redraw
.c3 adapath mesg slowopen
.c3 autoprint modeline term
.c3 beautify open terse
.c3 directory optimize ttytype
.c3 edcompatible prompt window
.c3 hardtabs
.R
.DE
.KE
.KS
while these parameters may well be implemented at some point:
.DS
.B
.c3 autowrite scroll warn
.c3 errorbells sourceany writeany
.R
.DE
.KE
.\"---------------------------------------------------------------------------
.NH 2
Miscellaneous
.LP
Some features of certain commands do not work as they should.
Specifically:
.IP \(bu 5
Repeat counts before insertions don't work.
.IP \(bu 5
Appending to named buffers is not yet implemented.
.IP \(bu 5
Typing \fB^Q\fP in input mode does not mean the same as \fB^V\fP;
it just inserts a `^Q', assuming it gets as far as the editor at all.
.IP \(bu 5
Typing \fB^W\fP in insert mode does not back up one word as in \fBvi\fP.
.IP \(bu 5
It is not possible to interrupt the editor while it is
performing certain operations.
If you start off a big global command, you have to wait for it to finish.
.IP \(bu 5
Flags and counts after colon commands are not supported.
.KS
.IP \(bu 5
It is not possible to read the output of a system command using
.DS
\fB:r\ !\fP\fIcommand\fP
.DE
or to write into a command using
.DS
\fB:w\ !\fP\fIcommand\fP
.DE
.KE
.IP \(bu 5
The \fB:substitute\fP command does not support splitting of lines.
.IP \(bu 5
Regular expressions, although implemented (see later for more details),
do not support the \fB~\fP character;
also, the \fB\e\^u\fP and \fB\e\^l\fP escape sequences are not supported on
the right-hand side of a substitute replacement pattern.
.IP \(bu 5
The \fB:global\fP command only supports the commands [\fBlps&~d\fP].
.IP \(bu 5
\fBUndo\fP does not work properly when applied to macros (either
.B @
or
\fB:map\fP); it should undo all the changes made by the macro, but in
fact only the last command within the macro is undone.
.\"===========================================================================
.NH 1
Enhancements
.LP
The following extensions are available in \fBxvi\fP.
.\"---------------------------------------------------------------------------
.KS
.NH 2
Parameter Handling
.LP
\fBXvi\fP supports 5 types of parameter:
as well as \fBvi\fP's
\fInumeric\fP,
\fIstring\fP and
\fIboolean\fP,
it also has
\fIenumerated\fP and
\fIlist\fP types.
The former is used for e.g. \fBformat\fP and \fBregextype\fP,
while the latter is currently only used for \fBtags\fP.
The advantage of the \fIenumerated\fP type is that if you try
to set an illegal value, the set of correct values will be displayed,
which is very useful if you have forgotten what the values may be.
(Try \fB:set preserve\fP to see an example of this.)
.KE
.\"---------------------------------------------------------------------------
.NH 2
Windows
.LP
\fBXvi\fP supports multiple \fIbuffers\fP and \fIwindows\fP.
A \fIbuffer\fP is the object which holds a file in memory,
while a \fIwindow\fP is an area of the screen which shows part of a buffer.
Note that every window references a buffer, even if no file is being edited.
.LP
The following commands are available for operating on buffers and windows:
.IP \fB:buffer\fP 10
create a new buffer in a new window; can be followed
by a filename, which will be edited in the new buffer.
.IP \fB:split\fP 10
create a new window onto the current buffer by
splitting the current window in half.
The two resulting windows are similar to
.I viewports
on to a single editing buffer,
in that changes made in one window are reflected in the other one.
.IP \fB:close\fP 10
close the current window; will also close the buffer
if this is the last window onto it.
.IP "\fB:x / ZZ\fP" 10
close only the current window.
If the window is the only one onto the buffer,
the buffer will be closed as well,
writing it first if it is modified.
Hence, for a single window, this command does the
same as in \fBvi\fP.
.IP \fBg\fP 10
move to the next window.
This is normally the window directly below the current one
on the screen, or the top window on the screen if the current
window is the bottom one.
.IP \fB^W\fP 10
increase the size of the current window (may be
given a numeric prefix, default is one line).
.IP \fB^T\fP 10
decrease the size of the current window (may be
given a numeric prefix, default is one line).
.IP \fB^O\fP 10
make the current window as large as possible.
.IP \fB^]\fP 10
as for \fBvi\fP, but creates a new buffer window
if appropriate (and if \fBautosplit\fP allows).
.LP
Note that the \fB:quit\fP command quits out of the editor,
not out of a window.
The \fB:close\fP command is thus the equivalent of \fB:quit\fP for windows.
There is no equivalent of \fB:x\fP or \fBZZ\fP for the whole editor;
these have been hijacked for operations on windows.
.LP
Also the numeric \fBautosplit\fP parameter specifies the maximum number
of buffer windows that will be created automatically whenever you
either edit more than one file, or use tags to edit a different file.
.LP
Undo works per buffer, as do marks; yank/put and redo (the \fB.\fP command)
work over all buffers, i.e. you can delete from one buffer and put
the text into a different buffer.
.LP
The \fBminrows\fP parameter specifies the minimum number of rows
to which a window may be shrunk, including the status line.
The default value is 2; 0 and 1 may also be useful.
.\"---------------------------------------------------------------------------
.KS
.NH 2
Named Buffers
.LP
As well as the normal named (conjugate) buffers, and the default one
named \fB@\fP,
several extra buffers named
.B : ,
.B / ,
.B ?
and
.B !
contain the last command lines entered for each of the command types.
So for instance,
.B @:
will re-execute the last colon command, or you can insert
it into your buffer, edit it and then re-execute it (e.g. with
.B dd@@ ).
.KE
.\"---------------------------------------------------------------------------
.KS
.NH 2
File Formats
.LP
\fBXvi\fP will handle different file formats, via the \fBformat\fP parameter,
which may be set to e.g.
.qB unix ,
.qB msdos ,
etc.
This means you can edit \%MS-DOS files under UNIX, etc.
.KE
.\"---------------------------------------------------------------------------
.KS
.NH 2
Regular Expressions
.LP
\fBVi\fP's \fBmagic\fP parameter is replaced by the \fBregextype\fP parameter,
which can take the following values:
.KE
.IP \fBtags\fP 10
only
.B ^
and
.B $
are significant (used for tags).
.IP \fBgrep\fP 10
like
.B grep (1),
but with
.B \e<
and
.B \e\^>
added.
.IP \fBegrep\fP 10
like
.B egrep (1),
but with
.B \e<
and
.B \e\^>
added.
.LP
The default is
.B grep .
.LP
The \fBsections\fP and \fBparagraphs\fP parameters define
.B egrep -style
patterns to search for, rather than
.B vi 's
simplistic (and
.B troff -dependent)
character pairs.
.LP
The \fBsentences\fP parameter is also implemented in this fashion,
but this is not completely satisfactory at the moment.
.LP
Note that it is possible to set or unset the \fBmagic\fP parameter
as in \fBvi\fP; this will simply result in the \fBregextype\fP parameter
being set as appropriate.
.\"---------------------------------------------------------------------------
.KS
.NH 2
Colour
.LP
\fBXvi\fP has a minimal amount of support for colours.
Basically, there are considered to be four settable colours,
each with a numeric parameter to say what its value is:
.IP \fBcolour\fP 14
colour used for text
.IP \fBstatuscolour\fP 14
colour used for status lines
.IP \fBroscolour\fP 14
as statuscolour, but for readonly files
.IP \fBsystemcolour\fP 14
colour used for system mode
(i.e. subshells and after termination).
.KE
.LP
The actual values of these parameters are system-dependent;
on PC versions, they are hardware-dependent video attributes,
while on UNIX they are indexes into the
entries ``\fBc0\fP'' to ``\fBc9\fP''
in the
.B termcap (5)
database,
which are assumed to be colour-setting
escape sequences if they are present.
If they are not present,
.qB so
(begin standout mode)
and
.qB se
(end standout mode)
are used instead.
Values of 0 and 1 give normal text, 2 to 9 give standout mode.
.LP
The default colour for the \fBroscolour\fP parameter will usually involve red
if colours are available;
this is intended to provide a warning to the user that writing the file may
not be possible.
.KS
.LP
The colour values may be entered in decimal, octal or hexadecimal form.
This
may be convenient for PC versions where the numbers actually
represent colour bitmaps; for example, on \%MS-DOS,
.DS
.B
:set co=0x1f
.R
.DE
gives bright white text on a blue background.
.KE
.\"---------------------------------------------------------------------------
.KS
.NH 2
Replace Mode
.LP
\fBXvi\fP's \fIreplace\fP mode (entered by the \fBR\fP command)
acts more intelligently when you press the return key \(em
it leaves the rest of the current line alone, and just starts
replacing text on the next line, starting at the screen column
where you first typed \fBR\fP.
.KE
.\"---------------------------------------------------------------------------
.KS
.NH 2
Preserve
.LP
Rather than use \fBvi\fP's UNIX-specific method for preservation,
\fBxvi\fP does periodic preservation of all files
currently being edited into a temporary file in the same directory.
\fBXvi\fP tries to do this when you are not typing, so that you won't
notice the short delay when the temporary file is written out.
Obviously, only changed files are preserved in this way, and the
temporary file is normally removed
once the real file has been successfully written.
As an additional safety measure,
when a file is explicitly saved
and it appears not to have been preserved recently,
it is normally preserved first.
This ensures that,
even if the operating system crashes while the
real file is being created,
there should always be at least one recent copy of it in the filesystem.
The \fB:preserve\fP command is available as in \fBvi\fP to preserve
a specific buffer manually.
.KE
.LP
The level of safety provided by the preservation facility may be configured
by changing the values of the
.B preserve
and
.B preservetime
parameters.
The following values are available for
.B preserve :
.IP \fBunsafe\fP 10
Never preserve any buffer before an explicit save.
This can be useful on old, slow, floppy-only systems,
but is not generally recommended.
.IP \fBstandard\fP 10
The default value.
Only preserve a buffer before an explicit save if it appears not to have
been preserved recently.
.IP \fBsafe\fP 10
Always preserve buffers before they are written.
.IP \fBparanoid\fP 10
As for \fBsafe\fP, but the preserve file is never removed,
even after the file has been successfully written.
.LP
.nh
In all cases,
all modified buffers are preserved automatically after no user events
have been received for
.B preservetime
seconds,
if a minimum number of events (currently 60) have been received since the
last automatic preservation.
This behaviour can be more or less disabled by setting
.B preservetime
to a very high value.
(For example,
one of the authors sets it to 600 on the machine he uses at home,
which is an 8088-based PC with no hard disk;
by way of contrast,
on the SparcStation IPX which he uses at work,
he sets it to 2.)
.LP
The names given to preserve files are system-dependent,
but are generally of the form \*Q\fIfilename\fP.tmp\*U,
or \*Q\fIfilename\fP.001\*U to \*Q\fIfilename\fP.999\*U.
If a preserve file already exists, it will not be overwritten;
instead, a new filename will be generated.
.LP
The \fB\-r\fP command line option is not supported.
.\"---------------------------------------------------------------------------
.KS
.NH 2
Help
.LP
A primitive help facility is available; the command \fB:help\fP, also
mapped to the HELP or F1 keys on many systems, simply creates a new
buffer window onto a standard help file.
The name of the file which is edited is given by the \fBhelpfile\fP
string parameter;
the default on Unix versions is
\fB"/usr/lib/xvi.help"\fP.
.KE
.\"---------------------------------------------------------------------------
.KS
.NH 2
Scrolling
.LP
The
.B jumpscroll
enumerated parameter
controls
the editor's
behaviour when the cursor moves beyond the limits of
the current window.
If its value is
.B off ,
and the new position is still reasonably close to the current window,
the window will be scrolled so that the new cursor position is at the
top or bottom of it.
Typically, the window will be scrolled one
line at a time as the cursor is moved up or down.
This behaviour may not always be desirable; for example,
terminals without real scrolling regions
may force the editor to do a lot of screen updating,
possibly over a slow telephone line or overloaded network.
Also, on LCD screens or other displays with a long image persistence,
it may
make the text more difficult to read.
If
.B jumpscroll
is set to
.B on ,
scrolling behaviour is modified so that,
whenever
the cursor moves beyond the limits of the current window,
the window is redrawn so as to place the cursor as centrally as possible
in it; thus, the window appears to
.I jump
to the new position.
The default value for
.B jumpscroll
is \fBauto\fP,
which causes
the editor to jump
instead of scrolling only if it can't scroll the affected window
efficiently.
.LP
In all cases,
if the distance from the top or bottom of the window
to the new position is more than half the window size,
the editor will jump instead of scrolling.
.KE
.LP
Explicit scroll commands (e.g. \fB^D\fP) are not affected by
the \fBjumpscroll\fP parameter.
.\"---------------------------------------------------------------------------
.KS
.NH 2
8-bit Characters
.LP
Characters with the top bit set may be displayed, although
it is not yet possible to have null
(\(fm\^\e\^0\^\(fm)
bytes in a file buffer.
How the characters are displayed varies between systems;
on UNIX, they will be shown as an octal escape sequence,
while on \%MS-DOS, OS/2 and QNX they will be shown as the actual
character in the PC character set.
This can be controlled by setting the \fBcchars\fP and \fBmchars\fP
variables; if these parameters are set,
control- and meta-characters (respectively) are shown directly,
otherwise they are shown as some sequence of printable characters.
.LP
You can use the \fB^_\fP (control-underscore) command to flip the
top bit of the character the cursor is on.
This may be useful on systems where it is otherwise impossible
to enter 8-bit characters.
.LP
Tabs are normally displayed as a series of spaces of the appropriate
length (according to the \fBtabstops\fP parameter);
setting \fBlist\fP mode will cause them to be displayed as a control
character, as will unsetting the \fBtabs\fP parameter.
How the tab character is displayed is then under the control of
the \fBcchars\fP parameter.
.KE
.\"---------------------------------------------------------------------------
.KS
.NH 2
Mouse Support
.LP
Some mouse support is available for micro-based systems and workstations.
Clicking the mouse button on:
.IP "any line outside current window"
changes current window to the one indicated by the mouse
(can be used instead of \fBg\fP).
.IP "top line of any window"
scrolls window downwards (same as \fB^Y\fP).
.IP "bottom line of any window"
scrolls window upwards (same as \fB^E\fP).
.IP "status line of any window"
shows current file and lines (same as \fB^G\fP).
.IP "any text line of any window"
moves text cursor as near as possible to mouse cursor.
.LP
Also,
windows can be resized by \*Qdragging\*U the appropriate status line
up or down with the mouse.
.KE
.\"---------------------------------------------------------------------------
.KS
.NH 2
Miscellaneous
.IP \(bu 5
The \fB:wn\fP (write file and edit next) command is provided, as per PC-vi.
.KE
.IP \(bu 5
There is no limit to the number or size of \fBmap\fPs which may be defined,
nor is there any fixed limit to the number of lines in a buffer.
.IP \(bu 5
The \fBedit\fP parameter controls whether a buffer can be modified.
This may be used, together with the
.B readonly
parameter,
to implement a nicer version of
.B view (1)
than
\fBvi\fP's version, since it won't fool you into thinking that editing
the buffer is in any way safe.
Be warned: once having set \fBnoedit\fP, it is not possible to do a
\fB:set edit\fP.
It's a one-way street.
.IP \(bu 5
The \fBtimeout\fP parameter is implemented as a numeric value,
specifying the number of milliseconds after which to assume that
no further input is available to continue with the parsing of a
\fBmap\fP sequence.
This replaces \fBvi\fP's boolean parameter of the same name.
.IP \(bu 5
The \fBvbell\fP parameter may be used to specify use of a visual,
rather than audible, bell, if this is available.
.IP \(bu  5
The \fB:echo\fP command is available; it simply echoes its arguments,
after expansion of % and # characters.
.IP \(bu 5
In insert and replace modes,
.B ^A
has the same meaning as
.B ^@
in vi, except that it
works at any time,
not just for the first character.
Also, typing
\fB^B\fP\fIx\fP,
where
.I x
is the name of a conjugate buffer, inserts the contents of that
buffer into the input stream at that point.
The buffer named
.B <
always contains the last thing inserted,
so that
.B ^B<
is the same as
.B ^A .
.\"===========================================================================
.KS
.NH 1
Differences
.IP \(bu 5
Argument handling is somewhat different; for instance,
.B \-R
is not supported, but
.qB "\-s\ parameter=value"
is, which is much more useful anyway.
.KE
.IP \(bu 5
The
.B XVINIT
environment variable is read instead of
.B EXINIT .
Whilst no files are sourced
automatically,
users who wish to have a startup file can arrange it very easily.
.B sh (1)
or
.B ksh (1)
users should add this line to their
.B "$HOME/.profile" :
.DS
.B
XVINIT=\(fmsource \fIxvi-startup-file\fP\|\(fm; export XVINIT
.R
.DE
.B csh (1)
users should add this to their
.B "$HOME/.login" :
.DS
.B
setenv XVINIT \(fmsource \fIxvi-startup-file\fP\|\(fm
.R
.DE
and \%MS-DOS users should add this to their
.B autoexec.bat :
.DS
.B
set XVINIT=source \fIxvi-startup-file\fP
.R
.DE
.IP \(bu 5
The \fBtags\fP parameter can be used to specify multiple tags files;
these can be separated by either
.qB "\^\e\ \|"
(backslash space) or
.qB ","
(comma).
.IP \(bu 5
Alternate files are handled slightly differently,
owing to the presence of buffer and window handling.
Essentially, when you close a buffer, its filename
is remembered as the alternate file;
when you invoke the \fB^^\fP or \fB:e #\fP commands,
this filename is re-edited.
Note that \fB^^\fP edits the alternate file in a new buffer window.
.IP \(bu 5
Hitting the escape key while in the command line does not terminate input;
instead, it cancels input, returning the prompt to the beginning
of the line.
This applies to input for
.B : ,
.B / ,
.B ?
and
.B ! .
.IP \(bu 5
Character-based yanks (or deletes) which span line boundaries are handled
correctly (\fBvi\fP gets this wrong).
