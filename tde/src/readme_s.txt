
                         TDE, the Thomson-Davis Editor
                                  Version 5.1v
                                  May 1, 2007
                            Frank Davis / Jason Hood


    This is the source distribution of TDE, a public domain text (and
    binary) editor.  The manual and precompiled DOS and Windows binaries
    are available separately, from:

        http://www.geocities.com/jadoxa/tde/


    File Descriptions:

        readme_s.txt    this file
        bj_ctype.h      ctypes for non-English languages
        common.h        external global variable declarations
        config.h        structures for the config and syntax highlighting files
        */criterr.h     critical error info
        define.h        editor function defs
        dialogs.h       dialog defs
        filmatch.h      prototypes for the filename pattern matching functions
        unix/keys.h     translates characters & escape sequences to TDE keys
        letters.h       answers to prompts, etc...
        syntax.h        defines for syntax highlighting
        tdefunc.h       prototypes for all functions
        tdestr.h        defs for all structures and defines
        bj_ctype.c      ctype based on Byrial Jensen's ideas
        block.c         line, stream, and box block functions
        cfgfile.c       strings for configuration and syntax highlighting files
        config.c        reads and parses the configuration file
        */console.c     video and keyboard routines
        */criterr.c     critical error prompt and info
        default.c       default key assignments
        dialogs.c       defines the layout for all the dialogs
        diff.c          diff algorithms
        dirlist.c       directory list functions
        ed.c            basic editor functions
        file.c          reading and writing files
        filmatch.c      filename wildcard pattern matching
        findrep.c       Boyer-Moore search routines
        global.c        initial global variables and editor function array
        help.c          help screens
        hwind.c         initialization and display routines
        macro.c         keyboard macros
        main.c          main function and hardware routines
        memory.c        memory management functions
        menu.c          simple pop-up pull-down menus
        menucua.c       alternative menu layout
        movement.c      cursor and screen movement functions
        */port.c        routines for portability
        prompts.c       all user prompts
        pull.c          routines for handling the menu
        query.c         user response functions
        regx.c          NFA pattern matching machine
        sort.c          stable, non-recursive mergesort (and quicksort)
        syntax.c        syntax highlighting
        tab.c           entab and detab routines
        undo.c          undo and redo functions (not fully implemented)
        utils.c         misc. editor functions
        window.c        window routines
        wordwrap.c      word wrap functions
        dos/int24.asm   critical error replacement - interrupt 24
        dos/kbdint.asm  utility to simulate 101 scan codes on 83/84 keyboards
        tde.ico         icon for the Win32 version
        tde.rc          Win32 resource file
        makefile        make file -- djgpp, MinGW, Linux
        makefile.dos    make file -- MSC, BC, QuickC
        makefile.lcc    make file -- LCC-Win32
        tde.dsp         Visual Studio project file
        tde.shl         syntax highlighting configuration (with TDE source)
        tdedist.shl     distributed syntax highlighting configuration
        config.wsp      TDE workspace for adding new configuration item
        function.wsp    TDE workspace for adding new editor function
        diff.bat        batch file to run GNU diff configured for TDE
        tdv.exe         32-bit DOS stub
        tdv.cmd         Windows XP batch file
        update.txt      list of changes
        todo.txt        possible remaining changes

      config/
        tde.cfg         default configuration file
        tde.hlp         default help screens
        menu.cfg        default menu layout
        cua.cfg         alternative menu layout
        key.tdm         translates key presses to config strings
        mark.tdm        alternative method of marking blocks
        de.cfg          configuration file for German users
        tdecfg          default Linux configuration file
        tdehlp          default Linux help screens
        wordstar.cfg    WordStar emulation
        wordstar.txt    help file used by above


    Installation:

    Edit the appropriate make file for your system (see descriptions above)
    and check the settings are OK, then run the MAKE utility.  There should
    be no warnings (at least, for Borland, djgpp, MinGW and LCC-Win32; other
    compilers have not been tested); "make install" can now be used to place
    the executable in the desired location.  Note that TDEP.EXE and TDEW.EXE
    will be renamed to TDE.EXE.

    TDE has a viewer mode (everything is loaded read-only) which can be
    accessed by naming the executable TDV.  This can be achieved by simply
    copying the file, or using TDV.EXE, which is a stub to call TDE.EXE (the
    renamed TDEP.EXE).  Windows XP users (and possibly earlier NT-based
    versions) can use TDV.CMD, a batch file to explicitly call TDE.EXE with
    the viewer option.  NTFS users may like to create an actual link to TDE
    (if you have an appropriate utility to do so).

    To find the main configuration files, TDE will use the first of:
    TDEHOME, an environment variable pointing to a path; HOME, another
    environment variable; or the same path as the executable.  (However, in
    order for HOME to be used in DOS or Windows, it must contain a TDE.CFG
    file.)  Choose which method best suits you and place TDE.SHL there.  The
    default configuration can be found in CONFIG/TDE.CFG - modify this file
    to suit your tastes, then place it with TDE.SHL.  Similarly with the
    default help screens in CONFIG/TDE.HLP.  Linux users should use
    CONFIG/TDECFG and CONFIG/TDEHLP, and add a leading dot (ie: typically
    $HOME/.tdecfg and $HOME/.tdehlp).


    Hacking:

    Two workspaces are provided to assist in modifying the source.  These
    workspaces load all the files required to either add a new configuration
    setting (CONFIG.WSP) or to add a new editor function (FUNCTION.WSP).
    Both workspaces were created on a 47-line screen; use Zoom (Ctrl+F9) to
    resize the windows to your screen size.  They are also supplied as
    relative paths, but once saved the paths will become absolute.

    If you want to share your changes, please use the supplied DIFF.BAT
    file, with GNU diff.  Specify the original file first (either in a dir-
    ectory of its own, or renamed), then your modified file (which should
    keep the original name).  Extensive changes should be done using diff's
    -r option and using two directories.  DOS and Win9X users should use
    COMMAND to run DIFF.BAT, in order to be able to redirect the output (ie:
    command /c diff old new > diff.txt).  I'm afraid Linux users will have
    to create their own equivalent shell script.  For example:

        unzip tde51qs -d tde51q
        unzip tde51rs -d tde51r
        diff -rN tde51q tde51r >tde51qr.txt
