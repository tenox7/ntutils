#ifndef FTEVER_H
#define FTEVER_H

#define MAKE_VERSION(major,minor,release) ((major<<24L) | (minor << 16L) | release)

#define PROG_FTE      "fte"
#define PROG_CFTE     "cfte"
#define PROGRAM       PROG_FTE
#define EXTRA_VERSION "-cvs"
#define VERSION       "0.50.02" EXTRA_VERSION
#define VERNUM        MAKE_VERSION(0x00, 0x50, 0x0002)
#define COPYRIGHT     "Copyright (c) 1994-1998 Marko Macek\n" \
                      "Copyright (c) 2000-2010 Others"

#endif // FTEVER_H
