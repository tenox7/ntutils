/*    c_fconfig.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef C_FCONFIG_H
#define C_FCONFIG_H

#define CF_STRING   1
#define CF_INT      2
#define CF_REGEXP   3

#define CF_END      100
#define CF_SUB      101
#define CF_MENU     102
#define CF_OBJECT   103
#define CF_COMMAND  104
#define CF_ITEM     105
#define CF_SUBMENU  106
#define CF_MENUSUB  107
#define CF_MODE     108
#define CF_PARENT   109
#define CF_KEYSUB   110
#define CF_KEY      111
#define CF_COLOR    112
#define CF_KEYWORD  113
#define CF_SETVAR   114
#define CF_COMPRX   115
#define CF_EVENTMAP 116
#define CF_COLORIZE 117
#define CF_ABBREV   118
#define CF_HSTATE   119
#define CF_HTRANS   120
#define CF_HWORDS   121
#define CF_SUBMENUCOND 122
#define CF_HWTYPE   123
#define CF_VARIABLE 124
#define CF_CONCAT   125
#define CF_CVSIGNRX 126
#define CF_SVNIGNRX 127

#define CF_EOF      254

#define CONFIG_ID   0x1A1D70E1

#endif // C_FCONFIG_H
