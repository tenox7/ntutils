/*
 * If we are in a unix environment, lets map signals to our DOS critical
 * error handler.
 *
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
 * This code is released into the public domain, Frank Davis.
 *    You may distribute it freely.
 */

#include "tdestr.h"
#include "common.h"
#include "tdefunc.h"
#include "criterr.h"
#include <signal.h>


#include <setjmp.h>
extern jmp_buf editor_jmp;


/*
 * Save the area of the screen that will display the Critical
 * Error info.  CEH_WIDTH and CEH_HEIGHT are the dimensions of critical
 * error screen in criterr.h.
 */
#define CEH_ROW          5
#define CEH_COL          6
#define CEH_WIDTH       67
#define CEH_HEIGHT      11
#define CEH_INFO_ROW     9
#define CEH_INFO_COL    18


/*
 * buffer for ceh info screen.
 */
static Char ceh_buffer[CEH_HEIGHT+1][CEH_WIDTH+4];


/*
 * Name:    crit_err_handler
 * Purpose: Show user something is wrong and get a response
 * Date:    November 13, 1993
 * Notes:   I noticed that some signals in Linux
 */
void crit_err_handler( int sig )
{
int  c;
const char * const *pp;

   if (sig == SIGINT) {
      g_status.control_break = TRUE;
      return;
   }

   g_display.output_space = g_display.frame_space;
   save_area( (Char *)ceh_buffer, CEH_WIDTH, CEH_HEIGHT, CEH_ROW, CEH_COL );
   show_strings( criterr_screen, CEH_HEIGHT, CEH_ROW, CEH_COL );
   shadow_area( CEH_WIDTH, CEH_HEIGHT, CEH_ROW, CEH_COL );

   switch (sig) {
      case SIGABRT   : pp = sigabrt;   break;
      case SIGALRM   : pp = sigalrm;   break;
      case SIGCONT   : pp = sigcont;   break;
      case SIGFPE    : pp = sigfpe;    break;
      case SIGHUP    : pp = sighup;    break;
      case SIGILL    : pp = sigill;    break;
      case SIGIO     : pp = sigio;     break;
      case SIGPIPE   : pp = sigpipe;   break;
      case SIGPWR    : pp = sigpwr;    break;
      case SIGQUIT   : pp = sigquit;   break;
      case SIGSEGV   : pp = sigsegv;   break;
      case SIGTERM   : pp = sigterm;   break;
      case SIGTRAP   : pp = sigtrap;   break;
      case SIGTSTP   : pp = sigtstp;   break;
      case SIGTTIN   : pp = sigttin;   break;
      case SIGTTOU   : pp = sigttou;   break;
      case SIGURG    : pp = sigurg;    break;
      case SIGUSR1   : pp = sigusr1;   break;
      case SIGUSR2   : pp = sigusr2;   break;
      case SIGVTALRM : pp = sigvtalrm; break;
      case SIGWINCH  : pp = sigwinch;  break;
      case SIGXCPU   : pp = sigxcpu;   break;
      case SIGXFSZ   : pp = sigxfsz;   break;
      default :
         pp = NULL;
         break;
   }
   if (pp != NULL)
      show_strings( pp, 3, CEH_INFO_ROW, CEH_INFO_COL );

   xygoto( -1, -1 );
   c = get_response( NULL, 0, 0, 4, L_CONTINUE, 0,
                                    L_IGNORE,   1,
                                    L_EXIT,     2,
                                    L_ABORT,    3 );
   if (c > 1) {
      console_exit( );
      set_overscan_color( 0 );
      if (c == 2)
         exit( 1 );
      else {
         signal( sig, SIG_DFL );
         return;                /* let the system handle it */
      }
   }
   restore_area( (Char *)ceh_buffer, CEH_WIDTH, CEH_HEIGHT, CEH_ROW, CEH_COL );
   g_display.output_space = FALSE;

   signal( sig, crit_err_handler );
   if (c  &&  !(sig == SIGSEGV || sig == SIGILL || sig == SIGFPE))
      xygoto( g_status.current_window->ccol, g_status.current_window->cline );
   else
      longjmp( editor_jmp, 1 );
}
