/*
 * Instead of the Abort, Retry, Ignore thing, let's try to handle critical
 * errors within tde.  Give the user some indication of the critical error
 * and then find out what the user wants to do.
 *
 * If we are in a unix environment, lets map signals to our DOS critical
 * error handler.
 *
 * IMPORTANT:  This is a replacement for the standard DOS critical error
 * handler.  Since DOS is not re-entrant, do not call any functions that,
 * directly or indirectly, call DOS functions.  We are in some DOS function
 * when a critical error occurs.  Using BIOS and direct hardware I/O
 * functions, however, is allowed.
 *
 * The prototype for the critical error handler is
 *
 *            int  FAR crit_err_handler( void )
 *
 * The handler is explicitly declared as "FAR", because the assembly
 * routine is hard coded for a "FAR" function.  See the bottom of
 * int24.asm for more info.
 *
 * See (incidentally, these are the current references for MSDOS 6.0):
 *
 *   Microsoft Knowledge Base, "Action Taken on Abort, Retry, Ignore, Fail",
 *    Microsoft Corporation, Redmond, Wash., 1992, Document Number: Q67586,
 *    Publication Date:  March 24, 1993.
 *
 *   Microsoft Knowledge Base, "Extended Error Code Information",
 *    Microsoft Corporation, Redmond, Wash., 1992, Document Number: Q74463,
 *    Publication Date:  March 24, 1993.
 *
 *   Programmer's Reference Manual, Microsoft Corporation, Redmond,
 *    Washington, 1986, Document No. 410630014-320-003-1285, pp. 1-20 thru
 *    1-21, pp. 1-34 thru 1-38, p 1-99, pp. 1-121 thru 1-124, pp. 1-216 thru
 *    1-218, pp. 2-1 thru 2-30.
 *
 *   Ray Duncan, _Advanced MS-DOS_, Microsoft Press, Redmond, Washington,
 *    1986, ISBN 0-914845-77-2, pp 89-97, pp 130-133.
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
#include "dos/criterr.h"


/*
 * Save the area of the screen that will display the Critical
 * Error info.  CEH_WIDTH and CEH_HEIGHT are the dimensions of critical
 * error screen in criterr.h.   CEH_OFFSET is the offset into the screen
 * refresh buffer.
 */
#define CEH_ROW          5
#define CEH_COL          5
#define CEH_WIDTH       70
#define CEH_HEIGHT      17
#define CEH_INFO_ROW     9
#define CEH_INFO_COL    24


/*
 * buffer for ceh info screen.
 */
static Char ceh_buffer[CEH_HEIGHT+1][CEH_WIDTH+4];


/*
 * Name:    crit_err_handler
 * Purpose: Show user something is wrong and get a response
 * Date:    April 1, 1992
 */
int  crit_err_handler( void )
{
int  rc;

   xygoto( -1, -1 );
   g_display.output_space = g_display.frame_space;
   save_area( (Char *)ceh_buffer, CEH_WIDTH, CEH_HEIGHT, CEH_ROW, CEH_COL );
   show_error_screen( );
   rc = get_response( NULL, 0, 0, 3, L_ABORT, ABORT,
                                     L_FAIL,  FAIL,
                                     L_RETRY, RETRY );
   restore_area( (Char *)ceh_buffer, CEH_WIDTH, CEH_HEIGHT, CEH_ROW, CEH_COL );
   g_display.output_space = FALSE;
   xygoto( 0, g_display.mode_line );

   return( rc );
}


/*
 * Name:    show_error_screen
 * Purpose: Display error screen in window
 * Date:    April 1, 1992
 *
 * jmh 980809: rewrote using the array and show_strings function.
 */
void show_error_screen( )
{
char *drive = "?";
const char *ceh_info[8];

   if (ceh.dattr == 0)
      *drive = ceh.drive + 'a';
   else
       drive = (char*)notapp;

   ceh_info[0] = error_code[ceh.code];
   ceh_info[1] = operation[ceh.rw];
   ceh_info[2] = drive;
   ceh_info[3] = ext_err[ceh.extended];
   ceh_info[4] = error_class[ceh.class];
   ceh_info[5] = locus[ceh.locus];
   ceh_info[6] = device_type[ceh.dattr];
   ceh_info[7] = (ceh.dattr == 0) ? notapp : ceh.dname;

   show_strings( criterr_screen, CEH_HEIGHT, CEH_ROW, CEH_COL );
   show_strings( ceh_info, 8, CEH_INFO_ROW, CEH_INFO_COL );
   shadow_area( CEH_WIDTH, CEH_HEIGHT, CEH_ROW, CEH_COL );
}
