README.html file for elvis 2.2  

Quick intro to elvis 2.2, with links to source code and binaries
================================================================

**CONTENTS**

*   [1\. About this file](#thisfile)
*   [2\. Differences between vi and elvis 2.2](#vi)
*   [3\. Differences between 2.1 and 2.2](#new)
    *   [3.1 New EX commands](#NewEx)
    *   [3.2 New VI commands](#NewVi)
    *   [3.3 New options](#NewOptions)
    *   [3.4 Changed or removed options](#OldOptions)
*   [4\. Links to related files](#links)


 **This is not elvis' user manual!**  The _real_ documentation for   
 elvis is located in its online help facility.  While running  
 elvis, enter the command "`:help`" to see the table of contents.

1\. About this file
===================

This file is written in the HTML markup language. You can view it with any WWW viewer, such as Netscape. You can also use elvis 2.2 to view it; this version of elvis has the ability to view HTML documents, and print them.

If elvis 2.2 doesn't automatically start up in HTML mode when you view this file, then you'll need to force it into HTML mode by giving the command "`:display html`".

2\. Differences between vi and elvis 2.2
========================================

Elvis is a superset of vi. It runs on more operating systems than vi, it is free, and you can obtain the source code. Elvis also has many new features. These new features are described in the first chapter of the online manual, which hypertext links to the other parts of the manual where those features are described in detail. Here's a just brief list:

*   Multiple edit buffers, so you can edit several files at once.
*   Multiple windows, so you can see multiple edit buffers, or different parts of the same edit buffer.
*   Multiple user interfaces, including graphical interfaces under Windows95/98/NT and X11.
*   A variety of display modes, including syntax coloring and HTML.
*   Online help, with hypertext links.
*   Enhanced tags, to support overloading in C++.
*   Network support, so you can load/save files via FTP, or even use elvis as a light-weight Web browser.
*   Aliases, which allow you to define new ex commands.
*   Built-in calculator

For a more complete list, with links to detailed descriptions, check the online manual. You can bring up the online manual by starting elvis and giving the command "`:help`". The list of extensions appears after the table of contents and a brief introduction.

3\. Differences between 2.1 and 2.2
===================================

The following is a summary of changes made since the release of elvis 2.1. These are in addition to any bug fixes.

3.1 New EX commands
-------------------

:alias, :unalias

Create, list, alter, or remove aliases. Aliases are new ex commands that you create from a sequence of other ex commands.

:autocmd, :augroup

Create, list, alter, or remove autocmds. Autocmds are ex command lines that are executed automatically on certain events, such as loading a file.

:check, :words, :wordfile

These configure the built-in spell checker.

:color

Not really new, but radically changed. The new version is much more powerful.

:fold, :unfold

These define folds, and either close or open them. Folds give you a way to hide parts of your text without actually deleting it. There is a **:foldc** alias which folds C source pretty well.

:for, :do

This loops over a list of file names, or other values.

:local

Used within a script or alias, this creates a local copy of an option so the alias can use it for storing temporary values, without clobbering the value that the option had before.

:map

Not new, but significantly changed. You can now use keywords to control when and how the map is applied.

:nohlsearch

Temporarily removes the highlighting for the _hlsearch_ option. Then next time you search, the highlighting will return.

:normal

When invoked with arguments, Elvis will interpret the arguments as vi commands.

:only

Closes all windows except this one.

:preserve

Exit elvis, but don't delete the session file.

:push

This is similar to **:e**, except that **:push** saves the cursor's original location on the tag stack, so you can return via **:pop** or ^\].

:region, :unregion, :chgregion

These create, remove, or change highlighted regions. The text itself is unchanged. Generally you would use :region to highlight text that has changed, or is significant for some other reason.

:safely

Run an ex command line with the _security_ option temporarily increased to "safer". This replaces the **:safer** command which ran a script. You can achieve the same effect via "`:safely source` script".

:switch, :case, :default

The usual "switch" control structure. This is useful in scripts and aliases.

3.1 New VI commands
-------------------

^I (<Tab>)

For "html" mode, this moves forward to the next hypertext link. For "normal" and "syntax", it toggles folding of the current line.

g^I (g<Tab>)

Move backward to the preceding hypertext link.

g^V (g<C-V>)

If a rectangle is selected, go to the other corner in the same line.

g$

Move to the end of the current row, taking line wrap into account.

g%

If any text is selected, go to the other end.

g0

Move to the start of the current row, taking line wrap into account.

g=key

Operator. Replace old text with an equal number of key characters.

gD, gd

Go to global or local definition of the word at the cursor.

gI

Input at start of line, before indent.

gJ

Join lines without adding whitespace

gS

Move to the end of a spell checker word. The spell checker's definition of a word differs from the rest of Elvis in that spell checker words may contain an apostrophe, provided there are letters on both sides of the apostrophe.

gU

Operator. Converts text to all uppercase.

g^

Move to start of current row, after indent, talking line wrap into account.

gh, gl

Move left or right one character, skipping over hidden characters. This is handy when viewing text in a markup display mode, since it moves over the markups effortlessly.

gj, gk

Move down or up one row, taking line wrap into account.

gs

Move to the next misspelled word. If given a count, then before moving it tries to fix the current misspelled word using one of the listed alternatives.

gu

Operator. Converts text to all lowercase.

g~

Operator. Toggles text between uppercase and lowercase.

3.3 New Options
---------------

antialias, aasqueeze

For "x11" only, the **antialias** option controls whether elvis will use the Xft library to draw antialiased text. Antialiased fonts tend to leave a much larger gap between lines, so the **aasqueeze** option gives you a way to reduce that gap, and get more lines on the screen.

auevent, aufilename, and auforce

These options are only defined during an autocmd event. They describe the event.

background

When Elvis doesn't know the background color (which can only happen when using a text-based user interface such as "termcap"), it may use this option to help it choose a contrasting foreground color.

bb

This is a buffer option with no built-in purpose. You can use it to store attributes about the buffer, for use by your own maps and aliases.

binary

This is set to indicate that elvis was invoked with a "-b" flag. The default `elvis.brf` uses this to set the **readeol** option to "binary" when appropriate.

blinktime

For "x11", this controls the cursor's blink rate. Setting it to 0 will disable blinking.

cleantext

Controls when elvis erases old text. Its value is a comma-delimited set of keywords, which may include "short", "long", "bs", "input", and/or "ex".

eventerrors, eventignore

These affect the handling of autocmd events.

filenamerules

This controls the way file names are parsed and manipulated by Elvis. It's value is a comma-delimited list of keywords, which may include "dollar", "paren", "space", "special", "tilde", and "wildcard". Windows users may want to remove the "space" keyword, which will prevent elvis from parsing spaces as name delimiters; this will make it easier to enter names with spaces in them.

folding

This is a window option. It controls whether folding is active in that window.

guidewidth

This is a buffer option, which draws thin vertical lines between columns of text. Its value is a comma-delimited list of column widths, similar to the **tabstop** and **shiftwidth** options.

hllayers, hlobject

These two options work together to give you a way to highlight text objects around the cursor. For example, you can highlight the current line via "`:set hllayers=1 hlobject=al`".

hlsearch

This highlights all instances of text that matches the most recent search.

iconimage

This chooses an icon to be used for Elvis' windows.

includepath

In "syntax" display mode, clicking on a #include file will search for that file in the includepath.

incsearch

Causes searches to be incremental. This means that every time you hit a key while entering the search regular expression, Elvis will search on the partial expression entered so far.

listchars

Controls the appearance of special characters in **list** mode. You can also use it to define arrows to mark long lines when side-scrolling is active (i.e., the **wrap** option is off).

lpcontrast

For color printing, this enforces a minimum contrast by darkening light colors.

lpoptions

For the "ps" and "ps2" **lptype**s, this can be set to a variety of values to tweak the output. See "`:help set lpoptions`".

magicchar, magicname, magicperl

These control the parsing of regular expressions.

prefersyntax

You can set the **prefersyntax** option to one of "never", "writable", "local", or "always" to control when elvis should start displaying a file in the "syntax" display modes rather than one of the markup display modes such as "html". For example, after "`:set prefersyntax=writable`", whenever you edit an HTML file that is writable Elvis will start in "syntax html" mode, but readonly files (including anything read via the HTTP protocol) will still start in "html" display mode.

scrollbgimage

Elvis can use background images in the "windows" and "x11" user interfaces. This option controls whether the background image should scroll when the text scrolls.

scrollwheelspeed

For wheel mice, this controls the number of lines scrolled for each detent of the wheel.

security

This can be set to "normal" for no protection, "safer" to protect you from malicious writing by a trojan horse, or "restricted" to protect the system from malicious reading by you.

smartargs

This uses the "tags" file to find the arguments for functions. When you're in "syntax" display mode and type in a function named followed by a '(', Elvis looks up that function name and inserts the formal parameters after the cursor. You can then overtype the formal parameters with the actual parameters.

spell, spellautoload, spelldict, and spellsuffix

These control Elvis' built-in spell checker.

ttyitalic, ttyunderline

These can be used to disable certain attributes on terminals that don't support them very well. In particular, the text mode on color VGA screens don't show underlining by converting the background to red; this may interfere with your own choice for a background color.

tweaksection

This relaxes the definition of a "section" so that the `[[` and `]]` commands will work well even if you aren't in the habit of putting your outer { characters in the first column of a line.

ww

This is a window option with no built-in purpose. You can use it to store attributes about the window, for use by your own maps and aliases.

xencoding

The "x11" interface now allows you to specify fonts as "fontname\*size", like the "windows" interface. When using this notation, Elvis will use the value of **xencoding** as the last to elements of the font's long name. A typical value is "iso8859-1" to load Latin-1 fonts.

3.4 Changed or removed options
------------------------------

commentfont, stringfont, prepfont, keywordfont, functionfont, otherfont, and variablefont

The syntax display mode previously used these options to control the appearance of different parts of the language. Those options are no longer necessary since the `:color` command can directly assign attributes to text faces named "comment", "string", etc. Consequently, those options have been deleted.

boldstyle, emphasizedstyle, fixedstyle, italicstype, normalstyle, and underlinedstyle

Similarly, the "windows" interface used these options to control the attributes of fonts. The have been deleted, since `:color` does this now.

underline

The "x11" interface used to have an "underline" option to control the attributes of the "underline" font. This has been deleted.

normalfont

The "x11" interface's **normalfont** has been renamed to "_font_", to be more similar to the "windows" interface.

lppaper

The **lppaper** option has been replaced by a more versatile _lpoptions_ option.

showname, showcmd, showstack and showtag

These options have all been replaced by a single _show_ option.

safer

The **safer** option has been replaced by a _security_ option, which can be set to "normal", "safer", or "restricted".

tabstop, shiftwidth

These options have been improved. You can now set the value to a comma-delimited list of column widths.

4\. Links to related files
==========================

The main download site is [ftp.cs.pdx.edu](ftp://ftp.cs.pdx.edu/pub/elvis/). The files can also be found at Elvis' home page, [http://elvis.vi-editor.org/](http://elvis.vi-editor.org/)

Most of the following are binary files, not text or HTML files, so you can't view then with your Web browser. But you can use your browser to download the files. For Netscape, use <Shift-Click>; for MSIE, use <RightClick> and "download".

[untar.c](/pub/elvis/untar.c)

This is the complete source code for "untar", a little program which extracts files from a gzipped tar archive. Comments near the top of "untar.c" describe how to compile and use it. If you already have the gzip and tar utilities, then you don't need this.

[untardos.exe](ftp://ftp.cs.pdx.edu/pub/elvis/untardos.exe)

This is an MS-DOS executable, produced from the above "untar.c" file. It can also be run under Windows 3.1, in a Dos-prompt window. For brief instructions on how to use `untardos,` run it with no arguments.

[untarw32.exe](ftp://ftp.cs.pdx.edu/pub/elvis/untarw32.exe)

This is a Win32 executable, produced from the above "untar.c" file. It runs under WindowsNT and Windows95. It runs somewhat faster than the MS-DOS version. It also supports long file names. For brief instructions on how to use `untarw32,` run it with no arguments, in a text-mode window.

**NOTE:** MS-Windows95 and MS-DOS use incompatible methods for mapping long file names to short ones. So if you extract the files using `untarw32.exe`, DOS programs won't be able to find them with their expected names, and vice versa. Consequently, you must use `untardos.exe` to unpack `elvis-2.2_0-msdos.tar.gz`, and `untarw32.exe` to unpack `elvis-2.2_0-win32.tar.gz`. (Actually, I recently added a **\-m** flag which forces `untarw32.exe` to convert long file names to short ones using the MS-DOS method.)

[untaros2.exe](ftp://ftp.cs.pdx.edu/pub/elvis/untaros2.exe)

This is an OS/2 executable, produced from the above "untar.c" file. For brief instructions on how to use `untaros2,` run it with no arguments.

[elvis-2.2\_0.tar.gz](elvis-2.2_0.tar.gz)

This is a gzipped tar archive of the source code and documentation for Elvis 2.2 and its related programs.

[elvis-2.2\_0-msdos.tar.gz](elvis-2.2_0-msdos.tar.gz)

This archive contains the documentation and MS-DOS executables for Elvis 2.2.

[elvis-2.2\_0-win32.tar.gz](elvis-2.2_0-win32.tar.gz)

This archive contains the documentation and Win32 executables for Elvis 2.2. These were compiled and tested under Windows95, but should work under WindowsNT 3.51 (or later) as well.

[elvis-2.2\_0-os2.tar.gz](elvis-2.2_0-os2.tar.gz)

This archive contains the documentation and OS/2 executables for Elvis 2.2. **If this link is broken then look in Herbert's site, below.**

[http://www.fh-wedel.de/pub/fh-wedel/staff/di/elvis/00-index.html](http://www.fh-wedel.de/pub/fh-wedel/staff/di/elvis/00-index.html)

This is where the OS/2 maintainer stores his most up-to-date versions. It may be better than the `elvis-2.2_0-os2.tar.gz` file, above.