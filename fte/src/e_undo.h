/*    e_undo.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef E_UNDO_H
#define E_UNDO_H

/*
 * only core operations can be directly undone
 * - Insert # of Lines
 * - Delete # of Lines
 * - Insert # Line
 * - Delete Line Text
 * - Insert Line Text
 * - Positioning
 * - Block marking
 */
 
enum UndoCommands {
    ucInsLine,
    ucDelLine,
    ucInsChars,
    ucDelChars,

    ucJoinLine,
    ucSplitLine,

    ucPosition,
    ucBlock,
    ucModified,

    ucFoldCreate,
    ucFoldDestroy,
    ucFoldPromote,
    ucFoldDemote,
    ucFoldOpen,
    ucFoldClose,

    ucPlaceUserBookmark,
    ucRemoveUserBookmark
};

#endif // E_UNDO_H
