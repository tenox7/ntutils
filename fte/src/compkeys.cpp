/*
 * compkeys.cpp
 *
 * Copyright (c) 1998 by István Váradi
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "conkbd.h"
#include "console.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FTESL_KBDCTRL(x)    (x - 'a' +  1)
// *INDENT-OFF*
static unsigned get_linux_keycode(TKeyCode kcode)
{
        static const unsigned lnxkeycodes[] = {
                /* 32 */
                57,  2, 40,  4,  5,  6,  8, 40,

                /* 40 */
                10, 11,  9, 13, 51, 12, 52, 53,

                /* 48 */
                11,  2,  3,  4,  5,  6,  7,  8,

                /* 56 */
                9 , 10, 39, 39, 51, 13, 52, 53,

                /* 64 */
                3 , 30, 48, 46, 32, 18, 33, 34,

                /* 72 */
                35, 23, 36, 37, 38, 50, 49, 24,

                /* 80 */
                25, 16, 19, 31, 20, 22, 47, 17,

                /* 88 */
                45, 21, 44, 26, 86, 27,  7, 12,

                /* 96 */
                43, 30, 48, 46, 32, 18, 33, 34,

                /* 104 */
                35, 23, 36, 37, 38, 50, 49, 24,

                /* 112 */
                25, 16, 19, 31, 20, 22, 47, 17,

                /* 120 */
                45, 21, 44, 26, 86, 27, 43,  0,
        };

        TKeyCode key = keyCode(kcode)|(kcode&kfGray);

        switch(key) {
            case kbF1:
            case kbF2:
            case kbF3:
            case kbF4:
            case kbF5:
            case kbF6:
            case kbF7:
            case kbF8:
            case kbF9:
            case kbF10:          return 59 + (unsigned)key - kbF1;
            case kbF11:          return 87;
            case kbF12:          return 88;
            case kbHome:         return 102;
            case kbEnd:          return 107;
            case kbPgUp:         return 104;
            case kbPgDn:         return 109;
            case kbIns:          return 110;
            case kbDel:          return 111;
            case kbUp:           return 103;
            case kbDown:         return 108;
            case kbLeft:         return 105;
            case kbRight:        return 106;
            case kbEnter:        return 28;
            case kbEsc:          return 1;
            case kbBackSp:       return 14;
            case kbSpace:        return 57;
            case kbTab:          return 15;
            case kbCenter:       return 76;
            case kfGray|'/':     return 98;
            case kfGray|'*':     return 55;
            case kfGray|'+':     return 78;
            case kfGray|'-':     return 74;
            case kfGray|kbEnter: return 96;
            case kfGray|'.':     return 83;
            case kfGray|'7':
            case kfGray|kbHome:  return 71;
            case kfGray|'8':
            case kfGray|kbUp:    return 72;
            case kfGray|'9':
            case kfGray|kbPgUp:  return 73;
            case kfGray|'4':
            case kfGray|kbLeft:  return 75;
            case kfGray|'5':     return 76;
            case kfGray|'6':
            case kfGray|kbRight: return 77;
            case kfGray|'1':
            case kfGray|kbEnd:   return 79;
            case kfGray|'2':
            case kfGray|kbDown:  return 80;
            case kfGray|'3':
            case kfGray|kbPgDn:  return 81;
            default:
		if (key > 32 && key < 128)
		    return lnxkeycodes[key - 32];
		return 0;
        }
}
// *INDENT-ON*

struct keymapper {
        TKeyCode        kcode;
        const char*     kname;
};

static const keymapper speckeymap[]={
        { kbHome,   "Home"   },
        { kbEnd,    "End"    },
        { kbPgUp,   "PgUp"   },
        { kbPgDn,   "PgDn"   },
        { kbIns,    "Ins"    },
        { kbDel,    "Del"    },
        { kbUp,     "Up"     },
        { kbDown,   "Down"   },
        { kbLeft,   "Left"   },
        { kbRight,  "Right"  },
        { kbEnter,  "Enter"  },
        { kbEsc,    "Esc"    },
        { kbBackSp, "BackSp" },
        { kbSpace,  "Space"  },
        { kbTab,    "Tab"    },
        { kbCenter, "Center" },
};

static TKeyCode ftesl_getkeycode(const char* key)
{
        TKeyCode        kcode = 0;

        if ( (*key)=='\0') return 0;

        while (*(key+1)=='+') {
                switch (*key) {
                    case 'A':
                        kcode|=kfAlt; break;
                    case 'C':
                        kcode|=kfCtrl; break;
                    case 'S':
                        kcode|=kfShift; break;
                    case 'G':
                        kcode|=kfGray; break;
                    default:
                        return 0;
                        break;
                }
                key+=2;
        }

        if ( (*key)=='\0') return 0;

        if ( *(key+1)=='\0') {
                kcode|=*(const unsigned char *)key;
                return kcode;
        }
        
        for (size_t i = 0; i < FTE_ARRAY_SIZE(speckeymap); ++i)
                if (!strcmp(key, speckeymap[i].kname)) {
                        kcode|=speckeymap[i].kcode;
                        return kcode;
                }

        if ( *key == 'F' ) {
                key++;
                if ( *key>='1' && *key<='9' ) {
                        if ( *key == '1' && *(key+1)!='\0' ) {
                                key++;
                                switch (*key) {
                                    case '1':
                                        kcode|=kbF11;
                                        break;
                                    case '2':
                                        kcode|=kbF12;
                                        break;
                                    default:
                                        return 0;
                                        break;
                                }
                                return kcode;
                        }
                        kcode|=kbF1+(*key-'1');
                        return kcode;
                }
        }

        return 0;
}

static int ftesl_get_ctrlcode(TKeyCode key)
{
        TKeyCode        kcode = keyCode(key);

        switch(kcode) {
            case kbUp:     return FTESL_KBDCTRL('u');
            case kbDown:   return FTESL_KBDCTRL('d');
            case kbLeft:   return FTESL_KBDCTRL('l');
            case kbRight:  return FTESL_KBDCTRL('r');
            case kbCenter: return FTESL_KBDCTRL('x');
            case kbHome:   return FTESL_KBDCTRL('b');
            case kbEnd:    return FTESL_KBDCTRL('e');
            case kbPgUp:   return FTESL_KBDCTRL('p');
            case kbPgDn:   return FTESL_KBDCTRL('n');
            case kbIns:    return FTESL_KBDCTRL('q');
            case kbDel:    return FTESL_KBDCTRL('z');
            case kbBackSp: return FTESL_KBDCTRL('h');
            case kbTab:    return FTESL_KBDCTRL('i');
            case kbEnter:  return FTESL_KBDCTRL('m');
        }
        if (kcode>=kbF1 && kcode<=kbF12) return FTESL_KBDCTRL('f');
        else return (int)kcode;
        
}


int main(int argc, char* argv[])
{
        FILE*           fin;
        FILE*           fout;
        char            finname[255];
        char            foutname[255];
        char            linebuf[256];
        char*           lptr;
        char            keyspecbuf[32];
        char*           bufptr;
        int             err = 0;
        unsigned        linecnt = 0;
        unsigned        strcnt = 0;
        //int             opt;

        finname[0] = '\0';
        foutname[0] = '\0';

        printf("Linux keymap compiler for SLang FTE, Copyright (c) 1998 by István Váradi\n\n");

        if (argc<3) {
                fprintf(stderr, "Usage: compkeys infile outfile\n\n");
                fprintf(stderr, "    where:\n");
                fprintf(stderr, "      infile:     the file with the list of the keys\n");
                fprintf(stderr, "      outfile:    the name of the output keymap file\n");
                exit(-2);
        }

        strcpy(finname,  argv[1]);
        strcpy(foutname, argv[2]);
        
        fin = fopen(finname, "rt");

        if (fin==NULL) {
                fprintf(stderr, "Can't open input file '%s' for reading.\n", finname);
                return -1;
        }

        fout = fopen(foutname, "wb");

        if (fout==NULL) {
                fprintf(stderr, "Can't open output file '%s' for writing.\n", foutname);
                fclose(fin);
                return -1;
        }


        printf("Compiling from '%s' into '%s'.\n", finname, foutname);

        fprintf(fout, "############################\n");
        fprintf(fout, "# Keytable to use with FTE #\n");
        fprintf(fout, "#  generated by 'compkeys' #\n");
        fprintf(fout, "############################\n\n");

        err = 0;
        while (!err && fgets(linebuf, sizeof(linebuf), fin)==linebuf) {
                linecnt++;
                lptr  = linebuf;
                while (!err) {
                        while (*lptr != '\0' && strchr(" \t", *lptr))
                                lptr++;
                        if (*lptr == '#' || *lptr == '\0' || *lptr == '\n')
                                break;
                        
                        bufptr=keyspecbuf;
                        
                        while (*lptr != '\0' && !strchr(" \t\n", *lptr))
                                *(bufptr++)=*(lptr++);
                        *(bufptr++)='\0';
                        
                        TKeyCode    kcode = ftesl_getkeycode(keyspecbuf), kcode1;

                        if (kcode==0) {
                                err = 2;
                        } else {
                                
                                fprintf(fout, "\n# %s\n", keyspecbuf);
                                if (kcode&kfShift) fprintf(fout, "shift ");
                                if (kcode&kfCtrl)  fprintf(fout, "control ");
                                if (kcode&kfAlt)   fprintf(fout, "alt ");
                                fprintf(fout, "keycode %3u = F%u\n",
                                        get_linux_keycode(kcode), 100+strcnt);
                                fprintf(fout, "string F%u = \"\\033", 100+strcnt);
                                if (kcode&kfShift) fprintf(fout, "\\023");
                                if (kcode&kfCtrl)  fprintf(fout, "\\003");
                                if (kcode&kfAlt)   fprintf(fout, "\\001");

                                int    ccode = ftesl_get_ctrlcode(kcode);
                                fprintf(fout, "\\%03o", ccode);
                                
                                
                                if (ccode==FTESL_KBDCTRL('f')) {
                                        kcode1 = keyCode(kcode);
                                        switch(kcode1) {
                                            case kbF1:
                                            case kbF2:
                                            case kbF3:
                                            case kbF4:
                                            case kbF5:
                                            case kbF6:
                                            case kbF7:
                                            case kbF8:
                                            case kbF9:
                                                fprintf(fout, "%c", (int)'1'+int(kcode1-kbF1));
                                                break;
                                            case kbF10:
                                                fprintf(fout, "0"); break;
                                            case kbF11:
                                                fprintf(fout, "a"); break;
                                            case kbF12:
                                                fprintf(fout, "b"); break;
                                        }
                                }
                                
                                fprintf(fout, "\"\n");
                                strcnt++;
                        }
                }
        }

        fclose(fout);
        fclose(fin);

        if (err) {
                fprintf(stderr, "line %u: ", linecnt);
                switch (err) {
                    case 1:
                        fprintf(stderr, "syntax error");
                        break;
                    case 2:
                        fprintf(stderr, "invalid key specification: '%s'",
                                keyspecbuf);
                }
                fprintf(stderr, "\n");
                remove(foutname);
        } else {
                printf("\nDone.\n");
        }

        if (err) return -1; else return 0;
}
