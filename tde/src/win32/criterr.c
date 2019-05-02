/*
 * Use signals for Win32. There's probably a more appropriate way.
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
 * Name:    ctrl_break_handler
 * Purpose: Recognize Ctrl-Break
 * Author:  Jason Hood
 * Date:    August 10, 2002
 * 050705:  recognise the other types, close if nothing's modified.
 */
BOOL ctrl_break_handler( DWORD type )
{
register TDE_WIN *wp;
int  rc;

   if (type == CTRL_BREAK_EVENT) {
      g_status.control_break = TRUE;
      return( TRUE );
   }

   rc = FALSE;
   for (wp=g_status.window_list; wp != NULL; wp=wp->next) {
      if (wp->file_info->modified && !wp->file_info->scratch) {
         rc = TRUE;
         break;
      }
   }
   return( rc );
}


/*
 * Name:    crit_err_handler
 * Purpose: Show user something is wrong and get a response
 * Date:    July 21, 1997
 */
void crit_err_handler( int sig )
{
int  c;
const char * const *pp;
static int aborted = FALSE;     /* returning generates another signal */

   if (aborted)
      ExitProcess( 1 );

   g_display.output_space = g_display.frame_space;
   save_area( (Char *)ceh_buffer, CEH_WIDTH, CEH_HEIGHT, CEH_ROW, CEH_COL );
   show_strings( criterr_screen, CEH_HEIGHT, CEH_ROW, CEH_COL );
   shadow_area( CEH_WIDTH, CEH_HEIGHT, CEH_ROW, CEH_COL );

   switch (sig) {
      case SIGABRT:
         pp = sigabrt;
         break;
      case SIGFPE:
         pp = sigfpe;
         break;
      case SIGILL:
         pp = sigill;
         break;
      case SIGSEGV:
      default:
         pp = sigsegv;
         break;
   }
   show_strings( pp, 3, CEH_INFO_ROW, CEH_INFO_COL );

   xygoto( -1, -1 );
   c = get_response( NULL, 0, 0, 3, L_CONTINUE, 0,
                                    L_EXIT,     1,
                                    L_ABORT,    2 );
   if (c) {
      console_exit( );
      if (c == 1)
         ExitProcess( 1 );
      else {
         aborted = TRUE;
         return;                /* let the system handle it */
      }
   }
   restore_area( (Char *)ceh_buffer, CEH_WIDTH, CEH_HEIGHT, CEH_ROW, CEH_COL );
   g_display.output_space = FALSE;

   signal( sig, crit_err_handler );
   longjmp( editor_jmp, 1 );
}
