/*    c_color.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_mode.h"
#include "o_routine.h"

#include <stdio.h>

// *INDENT-OFF*
ChColor MsgColor[3] = { 0x07, 0x0B, 0x04 };

/* Status line */

ChColor hcStatus_Normal = 0x30;
ChColor hcStatus_Active = 0x70;

ChColor hcEntry_Field   = 0x07;
ChColor hcEntry_Prompt  = 0x0F;
ChColor hcEntry_Selection = 0x2F;

/* Plain */

ChColor hcPlain_Normal = 0x07;
ChColor hcPlain_Background = 0x07;
ChColor hcPlain_Selected = 0x30;
ChColor hcPlain_Markers = 0x03;
ChColor hcPlain_Found = 0x40;
ChColor hcPlain_Keyword = 0x0F;
#ifdef CONFIG_FOLDS
ChColor hcPlain_Folds[5] = { 0x0A, 0x0A, 0x0A, 0x0A, 0x0A };
#endif
ChColor hcPlain_HilitWord = 0x0D;
#ifdef CONFIG_BOOKMARKS
ChColor hcPlain_Bookmark = 0x20;
#endif

/* LIST */
//ChColor hcList_Border       = 0x03;
ChColor hcList_Status       = 0x70;
ChColor hcList_Normal       = 0x07;
ChColor hcList_Selected     = 0x1F;
ChColor hcList_Hilited      = 0x0F;
ChColor hcList_HilitSelect  = 0x1F;
ChColor hcList_Marked       = 0xB0;
ChColor hcList_MarkSelect   = 0x1B;
ChColor hcList_MarkHilit    = 0xB1;
ChColor hcList_MarkHilitSel = 0x1B;

ChColor hcScrollBar_Arrows = 0x70;
ChColor hcScrollBar_Back   = 0x07;
ChColor hcScrollBar_Fore   = 0x07;

ChColor hcAsciiChars = 0x07;

ChColor hcMenu_Background = 0x70;
ChColor hcMenu_ActiveItem = 0x1F;
ChColor hcMenu_ActiveChar = 0x1C;
ChColor hcMenu_NormalItem = 0x70;
ChColor hcMenu_NormalChar = 0x74;

ChColor hcChoice_Title      = 0x1F;
ChColor hcChoice_Param      = 0x1B;
ChColor hcChoice_Background = 0x17;
ChColor hcChoice_ActiveItem = 0x20;
ChColor hcChoice_ActiveChar = 0x2F;
ChColor hcChoice_NormalItem = 0x1F;
ChColor hcChoice_NormalChar = 0x1E;

static const struct {
    const char* Name;
    ChColor* C;
} Colors[] = {
    { "Status.Normal",     &hcStatus_Normal },
    { "Status.Active",     &hcStatus_Active },

    { "Message.Normal",    &MsgColor[0] },
    { "Message.Bold",      &MsgColor[1] },
    { "Message.Error",     &MsgColor[2] },

    { "Entry.Field",       &hcEntry_Field },
    { "Entry.Prompt",      &hcEntry_Prompt },
    { "Entry.Selection",   &hcEntry_Selection },

    { "LIST.Status",       &hcList_Status },
    { "LIST.Normal",       &hcList_Normal },
    { "LIST.Selected",     &hcList_Selected },
    { "LIST.Hilited",      &hcList_Hilited },
    { "LIST.HilitSelect",  &hcList_HilitSelect },
    { "LIST.Marked",       &hcList_Marked },
    { "LIST.MarkSelect",   &hcList_MarkSelect },
    { "LIST.MarkHilit",    &hcList_MarkHilit },
    { "LIST.MarkHilitSel", &hcList_MarkHilitSel },

    { "PLAIN.Normal",      &hcPlain_Normal },
    { "PLAIN.Background",  &hcPlain_Background },
    { "PLAIN.Selected",    &hcPlain_Selected },
    { "PLAIN.Markers",     &hcPlain_Markers },
    { "PLAIN.Found",       &hcPlain_Found },
    { "PLAIN.Keyword",     &hcPlain_Keyword },
#ifdef CONFIG_FOLDS
    { "PLAIN.Folds",       &hcPlain_Folds[0] }, // added some backwards compatibility
    { "PLAIN.Folds0",      &hcPlain_Folds[0] },
    { "PLAIN.Folds1",      &hcPlain_Folds[1] },
    { "PLAIN.Folds2",      &hcPlain_Folds[2] },
    { "PLAIN.Folds3",      &hcPlain_Folds[3] },
    { "PLAIN.Folds4",      &hcPlain_Folds[4] },
#endif
    { "PLAIN.HilitWord",   &hcPlain_HilitWord },
#ifdef CONFIG_BOOKMARKS
    { "PLAIN.Bookmark",    &hcPlain_Bookmark },

#endif
    { "ScrollBar.Arrows",  &hcScrollBar_Arrows },
    { "ScrollBar.Back",    &hcScrollBar_Back },
    { "ScrollBar.Fore",    &hcScrollBar_Fore },

    { "ASCII.Chars",       &hcAsciiChars },

    { "Menu.Background",   &hcMenu_Background },
    { "Menu.ActiveItem",   &hcMenu_ActiveItem },
    { "Menu.ActiveChar",   &hcMenu_ActiveChar },
    { "Menu.NormalItem",   &hcMenu_NormalItem },
    { "Menu.NormalChar",   &hcMenu_NormalChar },

    { "Choice.Title",      &hcChoice_Title },
    { "Choice.Param",      &hcChoice_Param },
    { "Choice.Background", &hcChoice_Background },
    { "Choice.ActiveItem", &hcChoice_ActiveItem },
    { "Choice.ActiveChar", &hcChoice_ActiveChar },
    { "Choice.NormalItem", &hcChoice_NormalItem },
    { "Choice.NormalChar", &hcChoice_NormalChar },
};
// *INDENT-ON*

int SetColor(const char *ColorV, const char *Value) {
    unsigned int ColBg, ColFg;

    if (sscanf(Value, "%1X %1X", &ColFg, &ColBg) != 2)
        return 0;

    ChColor C = ChColor(ColFg | (ColBg << 4));
    for (size_t i = 0; i < FTE_ARRAY_SIZE(Colors); ++i) {
        if (strcmp(ColorV, Colors[i].Name) == 0) {
            *Colors[i].C = C;
            return 1;
        }
    }
    return 0;
}
