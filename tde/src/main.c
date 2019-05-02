/*******************  start of original comments  ********************/
/*
 * Written by Douglas Thomson (1989/1990)
 *
 * This source code is released into the public domain.
 */

/*
 * Name:    dte - Doug's Text Editor program - hardware dependent module
 * Purpose: This file contains all the code that needs to be different on
 *           different hardware.
 * File:    hwibm.c
 * Author:  Douglas Thomson
 * System:  This particular version is for the IBM PC and close compatibles.
 *           It write directly to video RAM, so it is faster than other
 *           techniques, but will cause "snow" on most CGA cards. See the
 *           file "hwibmcga.c" for a version that avoids snow.
 *          The compiler is Turbo C 2.0, using one of the large data memory
 *           models.
 * Date:    October 10, 1989
 * Notes:   This module has been kept as small as possible, to facilitate
 *           porting between different systems.
 */
/*********************  end of original comments   ********************/


/*
 * These routines were rewritten for Microsoft C.  They are pretty much system
 * dependent and pretty much Microsoft C dependent.  I also renamed this file
 * "main.c" - easier to find the main function.
 *
 * New editor name:  TDE, the Thomson-Davis Editor.
 * Author:           Frank Davis
 * Date:             June 5, 1991, version 1.0
 * Date:             July 29, 1991, version 1.1
 * Date:             October 5, 1991, version 1.2
 * Date:             January 20, 1992, version 1.3
 * Date:             February 17, 1992, version 1.4
 * Date:             April 1, 1992, version 1.5
 * Date:             June 5, 1992, version 2.0
 * Date:             October 31, 1992, version 2.1
 * Date:             April 1, 1993, version 2.2
 * Date:             June 5, 1993, version 3.0
 * Date:             August 29, 1993, version 3.1
 * Date:             November 13, 1993, version 3.2
 * Date:             June 5, 1994, version 4.0
 * Date:             December 5, 1998, version 5.0 (jmh)
 *
 * This modification of Douglas Thomson's code is released into the
 * public domain, Frank Davis.  You may distribute it freely.
 */


char *greatest_composer_ever = "W. A. Mozart, 1756-1791";


#include "tdestr.h"             /* tde types */
#include "common.h"
#include "define.h"
#include "tdefunc.h"

#if !defined( __DOS16__ )
 #include <signal.h>            /* unix critical errors */
 #if defined( __DJGPP__ )
   #define __dj_include_pc_h_   /* prevent inclusion of pc.h & it's getkey() */
   #include <dos.h>
 #endif
#else
 #include <dos.h>               /* for renaming files */
#endif


void default_twokeys( void );   /* defined in default.c */


/*
 * Disable globbing.  Unfortunately, can't automatically do this for UNIX, so
 * wildcards which include directory names will bring up the file list for
 * each directory (TDE's globber will ignore directories).
 */
#if defined( __DJGPP__ )
#include <crt0.h>
char** __crt0_glob_function( char *unused )
{
   return( NULL );
}
#elif defined( __MINGW32__ )
int  _CRT_glob = 0;
#endif


#if defined( __TURBOC__ )
extern unsigned _stklen = 10240;        /* 10K stack (jmh 020821) */
#endif

extern char *cmd_config;        /* defined in config.c */
extern char wksp_file[];        /* defined in file.c */
extern int  wksp_loaded;        /* defined in file.c */

char init_wd[PATH_MAX];         /* directory TDE started in (jmh 021021) */


/*
 * Name:    main
 * Purpose: To do any system dependent command line argument processing,
 *           and then call the main editor function.
 * Date:    October 10, 1989
 * Passed:  argc:   number of command line arguments
 *          argv:   text of command line arguments
 *
 * jmh 980807: translate argv[0] to the HOME/executable directory.
 * jmh 010528: recognize "tdv" as "tde -v";
 *             test for "-?" and "--help";
 *             added g_status.errmsg to explain why the editor didn't start.
 * jmh 010605: process -v and -i directly here;
 *             added "-i config_file" option.
 * jmh 020802: added "-w workspace" option.
 * jmh 021023: moved some stuff into the portable console_init() functions.
 * jmh 021024: recognise "-G?" as regx help.
 * jmh 040715: use TDEHOME environment variable for default file location;
 *             have DOS also HOME, and UNIX also use executable (in the
 *              unlikely event HOME is not defined).
 * jmh 050819: recognise "-??" as wildcard help and "--version".
 * jmh 051018: DOS/Win: assume if last arg ends in '"' it should really be '\'.
 */
int  main( int argc, char *argv[] )
{
char *argv0;
char *home;
static char path[PATH_MAX];
int  i;
int  viewer;

   /*
    * See if we were asked to display help.
    */
   if (argc == 2) {
      if (strcmp( argv[1], "/?" ) == 0  ||
          strcmp( argv[1], "-?" ) == 0  ||
          strcmp( argv[1], "--help" ) == 0) {
         puts( tde_help );
         return( 0 );
      }
      if (strcmp( argv[1], "--version" ) == 0) {
         printf( "%.*s", (int)(strchr( tde_help, '\n') - (char *)tde_help ) + 1,
                         tde_help );
         return( 0 );
      }
      if (stricmp( argv[1], "-g?" ) == 0  ||
          strcmp(  argv[1], "-??" ) == 0) {
         const char * const *help = (argv[1][1] == '?') ? wildcard_help
                                                        : regx_help;
         for (i = 0; help[i] != NULL; ++i)
            puts( help[i] );
         return( 0 );
      }
   }

#if defined( __WIN32__ )
   /*
    * As recommended by the Win32 documentation.
    */
   SetFileApisToOEM( );
   GetModuleFileName( NULL, path, sizeof(path) );
   argv0 = path;
#else
   argv0 = argv[0];
#endif

   /*
    * determine the name of the binary.  If it starts with tdv, place the
    * editor in viewer mode.
    */
   for (i = strlen( argv0 ); --i >= 0;)
      if (argv0[i] == '/'
#if !defined( __UNIX__ )
                          || argv0[i] == '\\' || argv0[i] == ':'
#endif
                                                                )
         break;
   ++i;
#if defined( __UNIX__ )
   viewer = (strncmp( argv0+i, "tdv", 3 ) == 0);
#else
   viewer = (strnicmp( argv0+i, "tdv", 3 ) == 0);
#endif

   home = getenv( "TDEHOME" );
   if (home == NULL)
      home = getenv( "HOME" );
   if (home != NULL) {
#if !defined( __UNIX__ )
      /*
       * check the existence of tde.cfg before using HOME in DOS/Windows.
       */
      char cfg[PATH_MAX];
      join_strings( cfg, home, "/"CONFIGFILE );
      if (file_exists( cfg ) == ERROR)
         home = NULL;
      else
#endif
      argv0 = home;
   }
   if (home == NULL) {
      if (i == 0)
         argv0[i++] = '.';
      argv0[i] = '\0';
   }

   get_full_path( argv0, path );
   i = strlen( path );
   if (path[i-1] != '/') {
      path[i] = '/';
      path[i+1] = '\0';
   }

#if !defined( __UNIX__ )
   i = strlen( argv[argc-1] ) - 1;
   if (argv[argc-1][i] == '"')
      argv[argc-1][i] = '\\';
#endif

   g_status.arg     = 1;
   g_status.argc    = argc;
   g_status.argv    = argv;
   g_status.argv[0] = path;

   /*
    * jmh 010605: process -v and -i.
    * jmh 020802: and -w; initialise the workspace to the directory TDE was
    *              started in, not whatever the current dir. happens to be.
    * jmh 020817: fix problem with missing -w argument (just ignore it).
    * jmh 021024: specifying a workspace with files will auto-save it.
    * jmh 021031: "-w" (without a filename) will use the global workspace.
    */
   get_full_path( WORKSPACEFILE, wksp_file );
   while (g_status.arg < g_status.argc && *g_status.argv[g_status.arg] == '-') {
      i = g_status.argv[g_status.arg][1];
      if (i != 'v' && i != 'i' && i != 'w')
         break;
      if (i == 'v')
         viewer = TRUE;
      else /* i == 'i' || i == 'w' */ {
         if (g_status.argv[g_status.arg][2] == 0)
            cmd_config = g_status.argv[++g_status.arg];
         else
            cmd_config = g_status.argv[g_status.arg] + 2;
         if (i == 'w') {
            if (cmd_config != NULL) {
               get_full_path( cmd_config, wksp_file );
               cmd_config = NULL;
            } else
               join_strings( wksp_file, g_status.argv[0], WORKSPACEFILE );
            wksp_loaded = TRUE;
         }
      }
      ++g_status.arg;
   }

#if !defined( __DOS16__ )

   /*
    * unix signals are kinda analagous to DOS critical errors.
    */

   signal( SIGABRT,   crit_err_handler );
   signal( SIGFPE,    crit_err_handler );
   signal( SIGILL,    crit_err_handler );
   signal( SIGINT,    crit_err_handler );
   signal( SIGSEGV,   crit_err_handler );
# if defined( __UNIX__ )
   signal( SIGALRM,   crit_err_handler );
   signal( SIGCONT,   crit_err_handler );
   signal( SIGHUP,    crit_err_handler );
   signal( SIGIO,     crit_err_handler );
   signal( SIGPIPE,   crit_err_handler );
   signal( SIGPWR,    crit_err_handler );
   signal( SIGQUIT,   crit_err_handler );
   signal( SIGTERM,   crit_err_handler );
   signal( SIGTRAP,   crit_err_handler );
   signal( SIGTSTP,   crit_err_handler );
   signal( SIGTTIN,   crit_err_handler );
   signal( SIGTTOU,   crit_err_handler );
   signal( SIGURG,    crit_err_handler );
   signal( SIGUSR1,   crit_err_handler );
   signal( SIGUSR2,   crit_err_handler );
   signal( SIGVTALRM, crit_err_handler );
   signal( SIGWINCH,  crit_err_handler );
   signal( SIGXCPU,   crit_err_handler );
   signal( SIGXFSZ,   crit_err_handler );
# elif defined( __DJGPP__ )
   _go32_dpmi_lock_code( crit_err_handler, (unsigned long)crit_err_handler_end -
                                           (unsigned long)crit_err_handler );
# endif

#else /* defined( __DOS16__ ) */

   /*
    * install and initialize our simple Critical Error Handler.
    */
   install_ceh( &ceh );
   CEH_OK;
#endif

   if (initialize( ) != ERROR) {
      g_status.viewer_mode = viewer;
      editor( );
   }
   terminate( );

   if (g_status.errmsg != NULL)
      fprintf( stderr, "%s.\n", g_status.errmsg );

   return( 0 );
}


/*
 * Name:    error
 * Purpose: To report an error, and wait for a keypress before continuing.
 * Date:    June 5, 1991
 * Passed:  kind:   an indication of how serious the error was:
 *                      INFO:    continue after pressing a key
 *                      WARNING: as INFO, but prefix message with "Warning: "
 *          line:    line to display message
 *          message: string to be printed
 * Notes:   Show user the message and wait for a key.
 *
 * jmh 980813: added INFO capability.
 * jmh 021031: removed FATAL.
 * jmh 021106: if message is too long, drop the warning/any key parts.
 */
void error( int kind, int line, const char *message )
{
char buff[MAX_COLS+2];          /* somewhere to store error before printing */
int  len;
DISPLAY_BUFF;

   if (g_status.macro_executing  &&  kind == WARNING  &&
       (g_status.current_macro->flag & NOWARN))
      return;

   len = strlen( message ) + strlen( main3 );
   if (len <= g_display.ncols) {
      if (kind == WARNING  &&  len + strlen( main2 ) < (size_t)g_display.ncols)
         /*
          * WARNING: message : press any key
          */
         combine_strings( buff, main2, message, main3 );
      else
         /*
          * message : press any key
          */
         join_strings( buff, message, main3 );
      message = buff;
   }

   SAVE_LINE( line );
   set_prompt( message, line );
   getkey( );
   RESTORE_LINE( line );

   if (g_status.wrapped) {
      g_status.wrapped = FALSE;
      show_search_message( CLR_SEARCH );
   }
}


/*
 * Name:    terminate
 * Purpose: To free all dynamic structures and unload anything we loaded
 * Date:    June 5, 1991
 */
void terminate( void )
{
register TDE_WIN *wp;           /* register for scanning windows */
TDE_WIN *w;                     /* free window */
register file_infos *fp;        /* register for scanning files */
file_infos *f;                  /* free files */
int  i;

   /*
    * free the file structures, if not already free.
    */
   fp = g_status.file_list;
   while (fp != NULL) {
      f  = fp;
      fp = fp->next;
      free( f );
   }

   /*
    * free the window structures, if not already free.
    */
   wp = g_status.window_list;
   while (wp != NULL) {
      w  = wp;
      wp = wp->next;
      free( w );
   }

   /*
    * free any character classes in the nfa's.
    */
   for (i=0; i < REGX_SIZE; i++) {
      if (sas_nfa.class[i] == nfa.class[i]  &&  nfa.class[i] != NULL)
         free( nfa.class[i] );
      else if (sas_nfa.class[i] != NULL)
         free( sas_nfa.class[i] );
      else if (nfa.class[i] != NULL)
         free( nfa.class[i] );
   }

   /*
    * free all the memory allocations.
    */
   if (g_status.memory != NULL) {
      for (i = g_status.mem_num; --i >= 0;)
         free( g_status.memory[i].base );
      free( g_status.memory );
   }

#if defined( __MSDOS__ )
   /*
    * jmh 021021: restore the initial cwd if tracking paths.
    */
   if (mode.track_path)
      set_current_directory( init_wd );
#endif

   /*
    * restore the overscan (border) color
    */
   if (g_display.adapter != MDA)
      set_overscan_color( g_display.old_overscan );

   console_exit( );
}


/*
 * Name:    initialize
 * Purpose: To initialize all the screen status info that is not hardware
 *           dependent, and call the hardware initialization routine to
 *           pick up the hardware dependent stuff.
 * Date:    June 5, 1991
 * Returns: [g_status and g_display]: all set up ready to go
 * Notes:   It is assumed that g_status and g_display are all \0's to begin
 *           with (the default if they use static storage). If this may
 *           not be the case, then clear them explicitly here.
 */
int  initialize( void )
{
int  i;
char file[PATH_MAX];

   /*
    * jmh 030331: check these assertions once here, instead of every time.
    */
   /*
    * 15 for the minimum window width, 2 each side for EOF character and space
    */
   assert( strlen( eof_text[0] ) <= 15-4 );
   assert( strlen( eof_text[1] ) <= 15-4 );
   assert( strlen( EOL_TEXT ) == 3 );

   /*
    * see if there's enough memory (jmh 011202)
    */
   if (init_memory( ) == ERROR)
      return( ERROR );

   /*
    * do the hardware initialization first.
    */
   hw_initialize( );

   /*
    * now, initialize the editor modes, pointers, and counters.
    */
   bm.search_defined        = ERROR;
   sas_bm.search_defined    = ERROR;
   g_status.sas_defined     = ERROR;
   g_status.sas_search_type = ERROR;

   regx.search_defined      = ERROR;
   sas_regx.search_defined  = ERROR;

   g_status.marked_file    = NULL;
   g_status.current_window = NULL;
   g_status.current_file   = NULL;
   g_status.window_list    = NULL;
   g_status.file_list      = NULL;
   g_status.language_list  = NULL;
   g_status.buff_node      = NULL;
   g_status.mstack         = NULL;
   g_status.rec_macro      = NULL;

   g_status.window_count    = 0;
   g_status.file_count      = 0;
   g_status.scratch_count   = 0;
   g_status.line_buff_len   = 0;
   g_status.tabout_buff_len = 0;
   g_status.jump_to         = 0;
   g_status.jump_col        = 1;
   g_status.jump_off        = 0;
   g_status.viewer_key      = FALSE;
   g_status.command         = 0;
   g_status.key_pressed     = 0;
   g_status.sas_rcol        = 0;
   g_status.sas_rline       = 0;
   g_status.recording_key   = 0;

   g_status.copied          = FALSE;
   g_status.wrapped         = FALSE;
   g_status.marked          = FALSE;
   g_status.macro_executing = FALSE;
   g_status.replace_defined = FALSE;

   g_status.screen_display = TRUE;

   g_status.sas_tokens[0] = '\0';
   g_status.rw_name[0]    = '\0';
   g_status.subst[0]      = '\0';

   /*
    * set the number of lines from one page that should still be visible
    *  on the next page after page up or page down.
    */
   g_status.overlap = 1;

   /*
    * Set the previous command and command count.
    */
   g_status.last_command  = ERROR;
   g_status.command_count = 0;

   g_status.errmsg = NULL;              /* jmh 010528 */

   g_option = g_option_all;             /* jmh 031026: for tabs if no files */

   ruler_win.rline = -1;                /* jmh 050720: no popup ruler */

   /*
    * initialize the nodes in the nfa.
    */
   for (i=0; i < REGX_SIZE; i++) {
      sas_nfa.node_type[i] = nfa.node_type[i] = 0;
      sas_nfa.term_type[i] = nfa.term_type[i] = 0;
      sas_nfa.c[i] = nfa.c[i] = 0;
      sas_nfa.next1[i] = nfa.next1[i] = 0;
      sas_nfa.next2[i] = nfa.next2[i] = 0;
      sas_nfa.class[i] = nfa.class[i] = NULL;
   }

   /*
    * set up default history lists (jmh 990425)
    */
   add_to_history( mode.stamp, &h_stamp );

   /*
    * create the default two-key assignments (jmh 980727)
    */
   default_twokeys( );

   /*
    * clear the screen and show the authors' names
    */
#if defined( __UNIX__ )
   bkgdset( tde_color_table[Color( Text )] );
#endif
   cls( );
   show_credits( );

   tdecfgfile( NULL );

   /*
    * jmh 990404: set the right cursor size, if need to prompt for file name.
    */
   set_cursor_size( mode.insert ? g_display.insert_cursor :
                                  g_display.overw_cursor );

   /*
    * jmh 021021: remember the initial cwd, for correct command line names.
    */
   get_current_directory( init_wd );

   /*
    * jmh 050711: check for the existence of the default help file.
    */
   if (file_exists( mode.helpfile ) != ERROR)
      get_full_path( mode.helpfile, mode.helpfile );
   else {
      join_strings( file, g_status.argv[0], mode.helpfile );
      if (file_exists( file ) != ERROR)
         strcpy( mode.helpfile, file );
   }

   return( OK );
}


/*
 * Name:    hw_initialize
 * Purpose: To initialize the display ready for editor use.
 * Date:    June 5, 1991
 *
 * jmh 980727: moved tdecfgfile to initialize().
 * jmh 990409: restructured.
 * jmh 991024: moved a bit more stuff into video_config().
 * jmh 020813: moved redirection testing from initialize().
 */
void hw_initialize( void )
{
struct vcfg cfg;        /* defined in tdestr.h */

   g_status.input_redir    = !isatty( fileno( stdin ) );
   g_status.output_redir   = !isatty( fileno( stdout ) );

   console_init( &cfg );

   /*
    * Use an integer pointer to go thru the color array for setting up the
    * various color fields.
    */
   g_display.color = colour[(cfg.color != FALSE)];
   if (g_display.adapter != MDA)
      set_overscan_color( Color( Overscan ) );

   /*
    * set up screen size
    */
   g_display.ncols     = cfg.cols;
   g_display.nlines    = cfg.rows;
   g_display.mode_line = g_display.nlines - 1;
   g_display.end_line  = g_display.mode_line - 1;

   /*
    * jmh 990213: set default cursor sizes.
    * jmh 990404: modified due to new medium size.
    */
   g_display.insert_cursor = g_display.cursor[mode.cursor_size & 3];
   g_display.overw_cursor  = g_display.cursor[mode.cursor_size >> 2];

   /*
    * jmh 991022: set default frame style and shadow.
    */
#if defined( __UNIX__ ) && !defined( PC_CHARS )
   g_display.frame_style = 0;   /* ASCII */
#else
   g_display.frame_style = 3;   /* Double line outside, single line inside */
#endif
   g_display.frame_space = TRUE;   /* Draw a space around frame */
   g_display.output_space = FALSE; /* but not yet */
   g_display.shadow = TRUE;        /* shadow_width is set in video_config() */
}


/*
 * Name:    get_help
 * Purpose: save the screen and display key definitions
 * Date:    June 5, 1991
 * Notes:   This routine is dependent on the length of the strings in the
 *          help screen.  To make it easy to load in a new help screen,
 *          the strings are assumed to be 80 characters long followed by
 *          the '\0' character.  It is assumed that each string contains
 *          exactly 81 characters.
 *
 * jmh 990430: modified the help screen to be two dimensional array of char,
 *              rather than one dimensional array of pointers; this was done
 *              to prevent the compiler merging duplicate strings.
 *             added the viewer help screen.
 * jmh 991022: made it a special case in show_help().
 * jmh 991110: also used for CharacterSet.
 * jmh 050710: process the normal help after viewer help here.
 */
int  get_help( TDE_WIN *window )
{
   if (show_help( ) == 1) {
      g_status.current_file->read_only = FALSE;
      show_help( );
      g_status.current_file->read_only = TRUE;
   }
   return( OK );
}


/*
 * Name:    show_credits
 * Purpose: display authors
 * Date:    June 5, 1991
 *
 * jmh 991020: used the help colour, added shadow.
 * jmh 991022: only display if no arguments given;
 *             moved it to show_help().
 * jmh 991101: no arguments or redirection.
 * jmh 021028: no workspace.
 */
void show_credits( void )
{
   if (g_status.arg >= g_status.argc  &&
       !g_status.input_redir && !g_status.output_redir  &&
       file_exists( wksp_file ) == ERROR)
      show_help( );
}
