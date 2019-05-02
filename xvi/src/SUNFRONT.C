#ifndef lint
static char *sccsid = "@(#)sunfront.c	2.1 (Chris & John Downey) 7/29/92";
#endif

/***

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    sunfront.c
* module function:
    Terminal interface module for SunView: front end program.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey
***/

#include "xvi.h"

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/icon.h>
#include <suntool/canvas.h>
#include <suntool/tty.h>
#include <sys/filio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vfork.h>

static short icon_image[] =
{
#   include "xvi.icn"
};

mpr_static(iconpr, 64, 64, 1, icon_image);

static Frame frame;
static Tty xviwin;

/*
 * Handle keyboard or mouse input.
 */
static Notify_value
ttyproc(win, event, na, type)
Tty			win;
Event			*event;
Notify_arg		na;
Notify_event_type	type;
{
    static Pixfont	*deffont = (Pixfont *) 0; /* default font */
    register int	evtcode;
    static char		seqbuf[(sizeof (unsigned int) * 12) + 7] =
						{ PREFIXCHAR };
    int			nchars;
    bool_t		done;
    static unsigned	buttonstate, prevx, prevy;
    unsigned		mx, my;

#define	BUTTONMASK(e)		(1 << ((e) - BUT(1)))

    if (deffont == NULL) {
	/*
	 * Get default font.
	 */
	deffont = pf_default();
    }

    evtcode = event_action(event);
    mx = event_x(event) / deffont->pf_defaultsize.x;
    my = event_y(event) / deffont->pf_defaultsize.y;
    nchars = 0;
    done = FALSE;
    if (evtcode == LOC_DRAG && buttonstate == BUTTONMASK(MS_MIDDLE)) {
	if (mx != prevx || my != prevy) {
	    /*
	     * Mouse drag event. Send a PREFIXCHAR,
	     * followed by 'm', followed by the starting
	     * row, finishing row, starting column &
	     * finishing column, in that order.
	     */
	    sprintf(&seqbuf[1], "m%x;%x;%x;%x;", prevy, my, prevx, mx);
	    nchars = strlen(seqbuf);
	    done = TRUE;
	    prevx = mx;
	    prevy = my;
	}
    } else if (event_is_down(event)) {
	if (event_is_button(event)) {
	    switch (evtcode) {
	    case MS_MIDDLE:
		prevx = mx;
		prevy = my;
		break;
	    case MS_RIGHT:
		/*
		 * Right button pressed. We have to
		 * send a PREFIXCHAR, followed by 'p',
		 * followed by the position of the
		 * mouse cursor in character
		 * co-ordinates: y first, then x.
		 */
		sprintf(&seqbuf[1], "p%x;%x;", my, mx);
		nchars = strlen(seqbuf);
		done = TRUE;
	    }
	    buttonstate |= BUTTONMASK(evtcode);
	} else {
	    /*
	     * nchars is the number of characters we have
	     * to send to xvi.main. In most of the cases
	     * we have to deal with here, this will be 2.
	     */
	    nchars = 2;
	    done = TRUE;
	    switch (evtcode) {
	    case PREFIXCHAR:
		seqbuf[1] = PREFIXCHAR;
		break;
	    case KEY_RIGHT(7):
		seqbuf[1] = 'H';
		break;
	    case KEY_RIGHT(8):
		seqbuf[1] = 'k';
		break;
	    case KEY_RIGHT(9):
		seqbuf[1] = CTRL('B');
		break;
	    case KEY_RIGHT(10):
		seqbuf[1] = '\b';
		break;
	    case KEY_RIGHT(12):
		seqbuf[1] = ' ';
		break;
	    case KEY_RIGHT(13):
		seqbuf[1] = 'L';
		break;
	    case KEY_RIGHT(14):
		seqbuf[1] = 'j';
		break;
	    case KEY_RIGHT(15):
		seqbuf[1] = CTRL('F');
		break;
	    default:
		nchars = 0;
		done = FALSE;
	    }
	}
    } else if (event_is_up(event) && event_is_button(event)) {
	if (evtcode == MS_RIGHT)
	    done = TRUE;
	buttonstate &= ~BUTTONMASK(evtcode);
    }
    if (nchars > 0) {
	ttysw_input(xviwin, seqbuf, nchars);
    }
    return done ?
	   NOTIFY_DONE :
	   notify_next_event_func(win, event, na, type);
}

/*
 * Read messages coming from back-end process.
 */
static Notify_value
readsocket(client, fd)
Notify_client	client;
int		fd;
{
    char c;

    while (read(fd, &c, 1) == 1) {
	if (c == 'q') {
	    (void) notify_set_input_func(client, NOTIFY_FUNC_NULL, fd);
	    window_set(frame, FRAME_NO_CONFIRM, TRUE, 0);
	    window_destroy(frame);
	    return NOTIFY_DONE;
	}

    }
    return NOTIFY_DONE;
}

static Notify_value
sigign(client, signum, when)
Notify_client		client;
int			signum;
Notify_signal_mode	when;
{
    return NOTIFY_IGNORED;
}

/*
 * Start up our back-end process and connect its standard input,
 * output & error files to the tty subwindow we've created for it.
 *
 * We use a pair of connected stream sockets to communicate with it:
 * it can reference its socket as file descriptor 3. Currently, this
 * is only used by the back-end process to tell us when to exit (by
 * sending the single character 'q').
 */
static void
forkmain(argv)
char	**argv;
{
    int		winfd;
    int		commsock[2];
    int		savefd[4];
    int		nbflag;
    int		i;
    char	*progname;


    for (i = 0; i <= 3; i++) {
	while ((savefd[i] = dup(i)) <= 3) {
	    ;
	}
    }
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, commsock) != 0) {
	fprintf(stderr, "%s: can't create socket\n", argv[0]);
	exit(1);
    }
    winfd = (int) window_get(xviwin, TTY_TTY_FD);
    if ((progname = strdup(argv[0])) == NULL) {
	progname = "xvi.sunview";
    }
    argv[0] = XVI_MAINPROG;
    switch (vfork()) {
    case 0:		/* This is the child process. */
	for (i = 0; i <= 2; i++) {
	    dup2(winfd, i);
	}
	dup2(commsock[1], 3);
	ioctl(winfd, FIOCLEX, 0);
	ioctl(commsock[0], FIOCLEX, 0);
	ioctl(commsock[1], FIOCLEX, 0);
	for (i = 0; i <= 3; i++) {
	    ioctl(savefd[i], FIOCLEX, 0);
	}
	execvp(argv[0], argv);
	fprintf(stderr, "%s: can't execute %s\n", progname, argv[0]);
	fflush(stderr);
	_exit(1);

    case -1:
	fprintf(stderr, "%s: vfork() failed\n", progname);
	fflush(stderr);
	_exit(1);

    default:	/* This is the parent process. */
	/*
	 * We should only reach this point after the
	 * child has called execvp() (or died).
	 */
	for (i = 0; i <= 3; i++) {
	    dup2(savefd[i], i);
	    close(savefd[i]);
	}
	close(commsock[1]);
	/*
	 * commsock[0] is our end of the socketpair.
	 * We have to make it non-blocking & register
	 * an input handler for it.
	 */
	nbflag = 1;
	ioctl(commsock[0], FIONBIO, &nbflag);
	(void) notify_set_input_func((Notify_client) xviwin,
					readsocket, commsock[0]);
    }
}

main(argc, argv)
int	argc;
char	**argv;
{
    Icon	xvicon;
    char	*label;

    if ((label = strrchr(argv[0], '/')) == NULL) {
	label = argv[0];
    } else {
	label++;
    }
    xvicon = icon_create(ICON_IMAGE, &iconpr, 0);
    frame = window_create(NULL,				FRAME,
			    FRAME_LABEL,		label,
			    FRAME_ICON,			xvicon,
			    FRAME_ARGC_PTR_ARGV,	&argc, argv,
			    WIN_ERROR_MSG,		"Can't create window",
			    FRAME_NO_CONFIRM,		TRUE,
			    FRAME_SUBWINDOWS_ADJUSTABLE,
			    FALSE,
			    0);
    xviwin = window_create(frame,			TTY,
			    TTY_ARGV,			TTY_ARGV_DO_NOT_FORK,
			    WIN_CONSUME_KBD_EVENTS,	WIN_RIGHT_KEYS,
			    0,
			    WIN_CONSUME_PICK_EVENTS,	WIN_MOUSE_BUTTONS,
			    WIN_UP_EVENTS,
			    LOC_DRAG,
			    0,
			    0);
    (void) notify_set_signal_func((Notify_client) xviwin, sigign,
		      SIGHUP, NOTIFY_ASYNC);
    (void) notify_set_signal_func((Notify_client) xviwin, sigign,
		      SIGINT, NOTIFY_ASYNC);
    (void) notify_set_signal_func((Notify_client) xviwin, sigign,
		      SIGQUIT, NOTIFY_ASYNC);
    forkmain(argv);
    notify_interpose_event_func(xviwin, ttyproc, NOTIFY_SAFE);
    window_main_loop(frame);
}
