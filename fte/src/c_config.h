/*    c_config.h
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef C_CONFIG_H
#define C_CONFIG_H

#include "sysdep.h"

extern int ScreenSizeX;
extern int ScreenSizeY;
extern int CursorInsSize[2];
extern int CursorOverSize[2];
extern bool CursorBlink;
extern int SystemClipboard;
extern int OpenAfterClose;
extern int ShowMenuBar;
extern int ShowToolBar;
extern int SelectPathname;
extern char DefaultModeName[32];
extern int WSStyleSearch;
extern char PrintDevice[MAXPATH];
extern char CompileCommand[256];
extern int KeepHistory;
extern int LoadDesktopOnEntry;
extern int SaveDesktopOnExit;
extern int KeepMessages;
extern int ScrollBorderX;
extern int ScrollBorderY;
extern int ScrollJumpX;
extern int ScrollJumpY;
extern int GUIDialogs;
extern int SevenBit;
extern int WeirdScroll;
extern int LoadDesktopMode;
extern char HelpCommand[128];
extern char *ConfigSourcePath;
extern char ConfigFileName[MAXPATH];
extern int IgnoreBufferList;
extern char CvsCommand[256];
extern char CvsLogMode[32];
extern char SvnCommand[256];
extern char SvnLogMode[32];
extern int ReassignModelIds;
extern int RecheckReadOnly;
extern char XShellCommand[256];
extern int ShowTildeFilesInDirList;

const char *GetGUICharacters(const char *which, const char *defChars);
int LoadConfig(int argc, char **argv, char *CfgFileName);
int GetIndentMode(const char *Str);
int GetHilitMode(const char *Str);
int UseDefaultConfig();
int AddCRegexp(int file, int line, int msg, const char *regexp);
int LoadFile(char *WhereName, char *CfgName);

#endif // C_CONFIG_H
