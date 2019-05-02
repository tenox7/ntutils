/*    c_color.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef C_COLOR_H
#define C_COLOR_H

#include "c_mode.h"

extern ChColor hcStatus_Normal;
extern ChColor hcStatus_Active;

extern ChColor hcPlain_Normal;
extern ChColor hcPlain_Background;
extern ChColor hcPlain_Selected;
extern ChColor hcPlain_Markers;
extern ChColor hcPlain_Found;
extern ChColor hcPlain_Keyword;
extern ChColor hcPlain_Folds[5];
extern ChColor hcPlain_HilitWord;
extern ChColor hcPlain_Bookmark;

extern ChColor hcEntry_Field;
extern ChColor hcEntry_Prompt;
extern ChColor hcEntry_Selection;

//extern ChColor hcList_Border;
extern ChColor hcList_Status;
extern ChColor hcList_Normal;
extern ChColor hcList_Selected;
extern ChColor hcList_Hilited;
extern ChColor hcList_HilitSelect;
extern ChColor hcList_Marked;
extern ChColor hcList_MarkSelect;
extern ChColor hcList_MarkHilit;
extern ChColor hcList_MarkHilitSel;

extern ChColor hcScrollBar_Arrows;
extern ChColor hcScrollBar_Back;
extern ChColor hcScrollBar_Fore;

extern ChColor hcAsciiChars;

extern ChColor hcMenu_Background;
extern ChColor hcMenu_ActiveItem;
extern ChColor hcMenu_ActiveChar;
extern ChColor hcMenu_NormalItem;
extern ChColor hcMenu_NormalChar;

extern ChColor hcChoice_Title;
extern ChColor hcChoice_Param;
extern ChColor hcChoice_Background;
extern ChColor hcChoice_ActiveItem;
extern ChColor hcChoice_ActiveChar;
extern ChColor hcChoice_NormalItem;
extern ChColor hcChoice_NormalChar;

extern ChColor MsgColor[3];

int SetColor(const char *ChColor, const char *Value);

#endif // C_COLOR_H
