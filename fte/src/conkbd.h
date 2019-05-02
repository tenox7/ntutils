/*    conkbd.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef CONKBD_H
#define CONKBD_H

#define kfAltXXX    0x01000000
#define kfModifier  0x02000000
#define kfSpecial   0x00010000
#define kfAlt       0x00100000
#define kfCtrl      0x00200000
#define kfShift     0x00400000
#define kfGray      0x00800000
#define kfKeyUp     0x10000000
#define kfAll       0x00F00000

#define isAltXXX(x) (((x) & (kfAltXXX)) != 0)
#define isAlt(x)  (((x) & kfAlt) != 0)
#define isCtrl(x)  (((x) & kfCtrl) != 0)
#define isShift(x) (((x) & kfShift) != 0)
#define isGray(x) (((x) & kfGray) != 0)
#define keyType(x) ((x) & kfAll)
#define keyCode(x) ((x) & 0x000FFFFF)
#define kbCode(x) (((x) & 0x0FFFFFFF) & ~(kfGray | kfAltXXX))
#define isAscii(x) ((((x) & (kfAlt | kfCtrl)) == 0) && (keyCode(x) < 256))
                                  
#define kbF1         (kfSpecial | 0x101)
#define kbF2         (kfSpecial | 0x102)
#define kbF3         (kfSpecial | 0x103)
#define kbF4         (kfSpecial | 0x104)
#define kbF5         (kfSpecial | 0x105)
#define kbF6         (kfSpecial | 0x106)
#define kbF7         (kfSpecial | 0x107)
#define kbF8         (kfSpecial | 0x108)
#define kbF9         (kfSpecial | 0x109)
#define kbF10        (kfSpecial | 0x110)
#define kbF11        (kfSpecial | 0x111)
#define kbF12        (kfSpecial | 0x112)

#define kbUp         (kfSpecial | 0x201)
#define kbDown       (kfSpecial | 0x202)
#define kbLeft       (kfSpecial | 0x203)
#define kbCenter     (kfSpecial | 0x204)
#define kbRight      (kfSpecial | 0x205)
#define kbHome       (kfSpecial | 0x206)
#define kbEnd        (kfSpecial | 0x207)
#define kbPgUp       (kfSpecial | 0x208)
#define kbPgDn       (kfSpecial | 0x209)
#define kbIns        (kfSpecial | 0x210)
#define kbDel        (kfSpecial | 0x211)

#define kbSpace      32

#define kbBackSp     (kfSpecial | 8)
#define kbTab        (kfSpecial | 9) 
#define kbEnter      (kfSpecial | 13)
#define kbEsc        (kfSpecial | 27)

#define kbAlt        (kfModifier | 0x301)
#define kbCtrl       (kfModifier | 0x302)
#define kbShift      (kfModifier | 0x303)
#define kbCapsLock   (kfModifier | 0x304)
#define kbNumLock    (kfModifier | 0x305)
#define kbScrollLock (kfModifier | 0x306)

#define kbPause      (kfSpecial | 0x401)
#define kbPrtScr     (kfSpecial | 0x402)
#define kbSysReq     (kfSpecial | 0x403)
#define kbBreak      (kfSpecial | 0x404)

#endif // CONKBD_H
