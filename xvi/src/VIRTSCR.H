/* Copyright (c) 1990,1991,1992 Chris and John Downey */
/***

* @(#)virtscr.h	2.3 (Chris & John Downey) 9/4/92

* program name:
    xvi
* function:
    PD version of UNIX "vi" editor, with extensions.
* module name:
    virtscr.h
* module function:
    Definition of the VirtScr class.
* history:
    STEVIE - ST Editor for VI Enthusiasts, Version 3.10
    Originally by Tim Thompson (twitch!tjt)
    Extensive modifications by Tony Andrews (onecom!wldrdg!tony)
    Heavily modified by Chris & John Downey

***/

/*
 * Type of an object whose pointers can be freely cast.
 */
#ifdef	__STDC__
#   define  genptr	void
#else
#   define  genptr	char
#endif

typedef struct virtscr {
    genptr	*pv_window;
    int		pv_rows;
    int		pv_cols;
/* public: */
    struct virtscr
		*(*v_new) P((struct virtscr *));
    void	(*v_close) P((struct virtscr *));

    int		(*v_rows) P((struct virtscr *));
    int		(*v_cols) P((struct virtscr *));

    void	(*v_clear_all) P((struct virtscr *));
    void	(*v_clear_line) P((struct virtscr *, int, int));

    void	(*v_goto) P((struct virtscr *, int, int));
    void	(*v_advise) P((struct virtscr *, int, int, int, char *));

    void	(*v_write) P((struct virtscr *, int, int, char *));
    void	(*v_putc) P((struct virtscr *, int, int, int));

    void	(*v_set_colour) P((struct virtscr *, int));
    int		(*v_colour_cost) P((struct virtscr *));

    void	(*v_flush) P((struct virtscr *));

    void	(*v_beep) P((struct virtscr *));

/* optional: do not use if NULL */
    void	(*v_insert) P((struct virtscr *, int, int, char *));

    int		(*v_scroll) P((struct virtscr *, int, int, int));

    void	(*v_flash) P((struct virtscr *));

    void	(*v_status) P((struct virtscr *, char *, char *, long, long));

    void	(*v_activate) P((struct virtscr *));
} VirtScr;

#define	VSrows(vs)			((*(vs->v_rows))(vs))
#define	VScols(vs)			((*(vs->v_cols))(vs))
#define	VSclear_all(vs)			((*(vs->v_clear_all))(vs))
#define	VSclear_line(vs, row, col)	((*(vs->v_clear_line))(vs, row, col))
#define	VSgoto(vs, row, col)		((*(vs->v_goto))(vs, row, col))
#define	VSadvise(vs, r, c, i, str)	((*(vs->v_advise))(vs, r, c, i, str))
#define	VSwrite(vs, row, col, str)	((*(vs->v_write))(vs, row, col, str))
#define	VSputc(vs, row, col, c)		((*(vs->v_putc))(vs, row, col, c))
#define	VSset_colour(vs, colour)	((*(vs->v_set_colour))(vs, colour))
#define	VScolour_cost(vs)		((*(vs->v_colour_cost))(vs))
#define	VSflush(vs)			((*(vs->v_flush))(vs))
#define	VSbeep(vs)			((*(vs->v_beep))(vs))
#define	VSinsert(vs, row, col, str)	((*(vs->v_insert))(vs, row, col, str))
#define	VSscroll(vs, start, end, n)	((*(vs->v_scroll))(vs, start, end, n))
