/*    o_directory.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include <stdio.h>
#include <time.h>

#include "o_directory.h"

#include "c_config.h"
#include "c_commands.h"
#include "c_history.h"
#include "i_modelview.h"
#include "i_view.h"
#include "log.h"
#include "s_files.h"
#include "s_string.h"
#include "s_util.h"

#ifdef CONFIG_OBJ_DIRECTORY
EDirectory::EDirectory(int createFlags, EModel **ARoot, char *aPath) :
    EList(createFlags, ARoot, aPath),
    Files(0),
    FCount(0),
    SearchLen(0)
{
    char XPath[MAXPATH];

    ExpandPath(aPath, XPath, sizeof(XPath));
    Slash(XPath, 1);
    Path = XPath;
    RescanList();
}

EDirectory::~EDirectory() {
    if (Files) {
        for (int i = 0; i < FCount; i++)
            delete Files[i];
        free(Files);
    }
}

EEventMap *EDirectory::GetEventMap() {
    return FindEventMap("DIRECTORY");
}

void EDirectory::DrawLine(PCell B, int Line, int Col, ChColor color, int Width) {
    char s[1024];

    MoveCh(B, ' ', color, Width);
    if (Files && Line >= 0 && Line < FCount) {
        int Year, Mon, Day, Hour, Min, Sec;
        struct tm *t;
        time_t tim;
        off_t Size = Files[Line]->Size();
	char SizeStr[16];
        char ModeStr[11];

        tim = Files[Line]->MTime();
        t = localtime(&tim);

        if (t) {
            Year = t->tm_year + 1900;
            Mon = t->tm_mon + 1;
            Day = t->tm_mday;
            Hour = t->tm_hour;
            Min = t->tm_min;
            Sec = t->tm_sec;
        } else {
            Year = Mon = Day = Hour = Min = Sec = 0;
        }

        if (Size >= 10 * 1024 * 1024) {
            Size /= 1024;
            if (Size >= 1024 * 1024)
                sprintf(SizeStr, "%7ldM", (long)(Size / 1024));
            else
                sprintf(SizeStr, "%7ldK", (long)Size);
        } else
            sprintf(SizeStr, "%8ld", (long) Size);

#ifdef UNIX
	if (Files[Line]->Mode() >= 0)
	{
	    const int mode = Files[Line]->Mode();
	    ModeStr[0] = mode & S_IFDIR ? 'd' : '-';
	    ModeStr[1] = mode & S_IRUSR ? 'r' : '-';
	    ModeStr[2] = mode & S_IWUSR ? 'w' : '-';
	    ModeStr[3] = mode & S_IXUSR ? 'x' : '-';
	    ModeStr[4] = mode & S_IRGRP ? 'r' : '-';
	    ModeStr[5] = mode & S_IWGRP ? 'w' : '-';
	    ModeStr[6] = mode & S_IXGRP ? 'x' : '-';
	    ModeStr[7] = mode & S_IROTH ? 'r' : '-';
	    ModeStr[8] = mode & S_IWOTH ? 'w' : '-';
	    ModeStr[9] = mode & S_IXOTH ? 'x' : '-';
	    ModeStr[10] = '\0';
	}
	else
            ModeStr[0] = '\0';
#endif

	int l = snprintf(s, sizeof(s),
#ifdef UNIX
                         "%10s "
#endif
			 "%04d/%02d/%02d %02d:%02d:%02d %s %s%c",
#ifdef UNIX
                         ModeStr,
#endif
			 Year, Mon, Day, Hour, Min, Sec, SizeStr,
			 Files[Line]->Name(),
			 (Files[Line]->Type() == fiDIRECTORY) ? SLASH : ' ');

        if (l > 0 && Col < l)
	    MoveStr(B, 0, Width, s + Col, color, Width);
    }
}

int EDirectory::IsHilited(int Line) {
    return (Line >= 0 && Line < FCount) ? Files[Line]->Type() == fiDIRECTORY : 0;
}

int _LNK_CONV FileNameCmp(const void *a, const void *b) {
    const FileInfo *A = *(const FileInfo **)a;
    const FileInfo *B = *(const FileInfo **)b;

    if (!(A->Type() == fiDIRECTORY) && (B->Type() == fiDIRECTORY))
        return 1;

    if ((A->Type() == fiDIRECTORY) && !(B->Type() == fiDIRECTORY))
        return -1;

    return filecmp(A->Name(), B->Name());
}

void EDirectory::RescanList() {
    char Dir[256];
    char Name[256];
    int DirCount = 0;
    unsigned long SizeCount = 0;
    FileFind *ff;
    FileInfo *fi;
    int rc;

    if (Files)
        FreeList();

    Count = 0;
    FCount = 0;
    if (JustDirectory(Path.c_str(), Dir, sizeof(Dir)) != 0) return;
    JustFileName(Path.c_str(), Name, sizeof(Name));

    // we don't want any special information about symbolic links, just to browse files
    ff = new FileFind(Dir, "*", ffDIRECTORY | ffHIDDEN | ffLINK);
    if (ff == 0)
        return ;

    rc = ff->FindFirst(&fi);
    while (rc == 0) {
	assert(fi != 0);

	if (
	    strcmp(fi->Name(), ".") != 0 &&
            ( ShowTildeFilesInDirList || fi->Name()[strlen(fi->Name())-1] != '~' )
	   ) {
            Files = (FileInfo **)realloc((void *)Files, ((FCount | 255) + 1) * sizeof(FileInfo *));
            if (Files == 0)
            {
                delete fi;
                delete ff;
                return;
            }

            Files[FCount] = fi;

            SizeCount += Files[FCount]->Size();
            if (fi->Type() == fiDIRECTORY && (strcmp(fi->Name(), "..") != 0))
                DirCount++;
            Count++;
            FCount++;
        } else
            delete fi;
        rc = ff->FindNext(&fi);
    }
    delete ff;

    {
        char CTitle[256];

        sprintf(CTitle, "%d files%c%d dirs%c%lu bytes%c%-200.200s",
                FCount, ConGetDrawChar(DCH_V),
                DirCount, ConGetDrawChar(DCH_V),
                SizeCount, ConGetDrawChar(DCH_V),
                Dir);
        SetTitle(CTitle);
    }
    qsort(Files, FCount, sizeof(FileInfo *), FileNameCmp);
    NeedsRedraw = 1;
}

void EDirectory::FreeList() {
    if (Files) {
        for (int i = 0; i < FCount; i++)
            delete Files[i];
        free(Files);
    }
    Files = 0;
    FCount = 0;
}

int EDirectory::isDir(int No) {
    char FilePath[MAXPATH];

    JustDirectory(Path.c_str(), FilePath, sizeof(FilePath));
    Slash(FilePath, 1);
    strlcat(FilePath, Files[No]->Name(), sizeof(FilePath));
    return IsDirectory(FilePath);
}

int EDirectory::ExecCommand(ExCommands Command, ExState &State) {
    switch (Command) {
    case ExActivateInOtherWindow:
        SearchLen = 0;
        Msg(S_INFO, "");
        if (Files && Row >= 0 && Row < FCount)
	    if (!isDir(Row))
                return FmLoad(Files[Row]->Name(), View->Next);
        return 0;

    case ExRescan:
        return RescanDir();

    case ExDirGoUp:
        SearchLen = 0;
        Msg(S_INFO, "");
        FmChDir(SDOT SDOT);
        return 1;

    case ExDirGoDown:
        SearchLen = 0;
        Msg(S_INFO, "");
        if (Files && Row >= 0 && Row < FCount) {
            if (isDir(Row)) {
                FmChDir(Files[Row]->Name());
                return 1;
            }
        }
        return 0;

    case ExDirGoto:
        SearchLen = 0;
        Msg(S_INFO, "");
        return ChangeDir(State);

    case ExDirGoRoot:
        SearchLen = 0;
        Msg(S_INFO, "");
        FmChDir(SSLASH);
        return 1;

    case ExDirSearchCancel:
        // Kill search when moving
        SearchLen = 0;
        Msg(S_INFO, "");
        return 1;

    case ExDirSearchNext:
        // Find next matching file, search is case in-sensitive while sorting is sensitive
        if (SearchLen) {
            for (int i = Row + 1; i < FCount; i++) {
                if (strnicmp(SearchName, Files[i]->Name(), SearchLen) == 0) {
                    Row = i;
                    break;
                }
            }
        }
        return 1;

    case ExDirSearchPrev:
        // Find prev matching file, search is case in-sensitive while sorting is sensitive
        if (SearchLen) {
            for (int i = Row - 1; i >= 0; i--) {
                if (strnicmp(SearchName, Files[i]->Name(), SearchLen) == 0) {
                    Row = i;
                    break;
                }
            }
        }
        return 1;

    case ExDeleteFile:
        SearchLen = 0;
        Msg(S_INFO, "");
        return FmRmDir(Files[Row]->Name());
    default:
        ;
    }
    return EList::ExecCommand(Command, State);
}

int EDirectory::Activate(int No) {
    SearchLen = 0;
    Msg(S_INFO, "");
    if (Files && No >= 0 && No < FCount) {
        if (isDir(No)) {
            FmChDir(Files[No]->Name());
            return 0;
        } else {
            return FmLoad(Files[No]->Name(), View);
        }
    }
    return 1;
}

void EDirectory::HandleEvent(TEvent &Event) {
    STARTFUNC("EDirectory::HandleEvent");
    int resetSearch = 0;
    EModel::HandleEvent(Event);
    switch (Event.What) {
    case evKeyUp:
        resetSearch = 0;
        break;
    case evKeyDown:
        LOG << "Key Code: " << kbCode(Event.Key.Code) << ENDLINE;
        resetSearch = 1;
        switch (kbCode(Event.Key.Code)) {
        case kbBackSp:
            LOG << "Got backspace" << ENDLINE;
            resetSearch = 0;
            if (SearchLen > 0) {
                SearchName[--SearchLen] = 0;
                Row = SearchPos[SearchLen];
                Msg(S_INFO, "Search: [%s]", SearchName);
            } else
                Msg(S_INFO, "");
            break;
        case kbEsc:
            Msg(S_INFO, "");
            break;
        default:
            resetSearch = 0; // moved here - its better for user
            // otherwice there is no way to find files like i_ascii
            if (isAscii(Event.Key.Code) && (SearchLen < ExISearch::MAXISEARCH)) {
                char Ch = (char) Event.Key.Code;
                int Found;

                LOG << " -> " << BinChar(Ch) << ENDLINE;

                SearchPos[SearchLen] = Row;
                SearchName[SearchLen] = Ch;
                SearchName[++SearchLen] = 0;
                Found = 0;
                LOG << "Comparing " << SearchName << ENDLINE;
                for (int i = Row; i < FCount; i++) {
                    LOG << "  to -> " << Files[i]->Name() << ENDLINE;
                    if (strnicmp(SearchName, Files[i]->Name(), SearchLen) == 0) {
                        Row = i;
                        Found = 1;
                        break;
                    }
                }
                if (Found == 0)
                    SearchName[--SearchLen] = 0;
                Msg(S_INFO, "Search: [%s]", SearchName);
            }
            break;
        }
    }
    if (resetSearch) {
        SearchLen = 0;
    }
    LOG << "SearchLen = " << SearchLen << ENDLINE;
}

int EDirectory::RescanDir() {
    char CName[256] = "";

    if (Row >= 0 && Row < FCount)
        strcpy(CName, Files[Row]->Name());
    Row = 0;
    RescanList();
    if (CName[0] != 0) {
        for (int i = 0; i < FCount; i++) {
            if (filecmp(Files[i]->Name(), CName) == 0)
            {
                Row = i;
                break;
            }
        }
    }
    return 1;
}

int EDirectory::FmChDir(const char *Name) {
    char Dir[256];
    char CName[256] = "";

    if (strcmp(Name, SSLASH) == 0) {
        JustRoot(Path.c_str(), Dir, sizeof(Dir));
    } else if (strcmp(Name, SDOT SDOT) == 0) {
        Slash(&Path[0], 0);
        JustFileName(Path.c_str(), CName, sizeof(CName));
        JustDirectory(Path.c_str(), Dir, sizeof(Dir));
    } else {
        JustDirectory(Path.c_str(), Dir, sizeof(Dir));
        Slash(Dir, 1);
        strlcat(Dir, Name, sizeof(Dir));
    }
    Slash(Dir, 1);

    Path = Dir;
    Row = 0;
    RescanList();
    if (CName[0] != 0) {
        for (int i = 0; i < FCount; i++) {
            if (filecmp(Files[i]->Name(), CName) == 0)
            {
                Row = i;
                break;
            }
        }
    }
    UpdateTitle();
    return 1;
}

int EDirectory::FmRmDir(char const* Name)
{
    char FilePath[MAXPATH];
    strlcpy(FilePath, Path.c_str(), sizeof(FilePath) - 2);
    Slash(FilePath, 1);
    strlcat(FilePath, Name, sizeof(FilePath));

    int choice = View->MView->Win->Choice(GPC_CONFIRM, "Remove File",
					  2, "O&K", "&Cancel",
					  "Remove %s?", Name);

    if (choice == 0) {
        if (unlink(FilePath) == 0) {
            // put the cursor to the previous row
            --Row;

            // There has to be a more efficient way of doing this ...
            return RescanDir();
        }
	Msg(S_INFO, "Failed to remove %s", Name);
	return 0;
    }

    Msg(S_INFO, "Cancelled");
    return 0;
}

int EDirectory::FmLoad(const char *Name, EView *XView) {
    char FilePath[256];

    JustDirectory(Path.c_str(), FilePath, sizeof(FilePath));
    Slash(FilePath, 1);
    strcat(FilePath, Name);
    return FileLoad(0, FilePath, NULL, XView);
}

void EDirectory::GetName(char *AName, size_t MaxLen) {
    strlcpy(AName, Path.c_str(), MaxLen);
    if (MaxLen)
	Slash(AName, 0);
}

void EDirectory::GetPath(char *APath, size_t MaxLen) {
    strlcpy(APath, Path.c_str(), MaxLen);
    if (MaxLen)
	Slash(APath, 0);
}

void EDirectory::GetInfo(char *AInfo, size_t /*MaxLen*/) {
    char buf[256] = {0};
    char winTitle[256] = {0};

    JustFileName(Path.c_str(), buf, sizeof(buf));
    if (buf[0] == '\0') // if there is no filename, try the directory name.
        JustLastDirectory(Path.c_str(), buf, sizeof(buf));

    if (buf[0] != 0) // if there is a file/dir name, stick it in here.
    {
        strlcat(winTitle, buf, sizeof(winTitle));
        strlcat(winTitle, "/ - ", sizeof(winTitle));
    }
    strlcat(winTitle, Path.c_str(), sizeof(winTitle));

    sprintf(AInfo,
            "%2d %04d/%03d %-150s",
            ModelNo,
            Row + 1, FCount,
            winTitle);
/*    sprintf(AInfo,
            "%2d %04d/%03d %-150s",
            ModelNo,
            Row + 1, FCount,
            Path);*/
}

void EDirectory::GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen) {

    if (!MaxLen)
	return;

    strlcpy(ATitle, Path.c_str(), MaxLen);

    char P[MAXPATH];
    strlcpy(P, Path.c_str(), sizeof(P));
    Slash(P, 0);

    JustDirectory(P, ASTitle, SMaxLen);
    Slash(ASTitle, 1);
}

int EDirectory::ChangeDir(ExState &State) {
    char Dir[MAXPATH];
    char Dir2[MAXPATH];

    if (State.GetStrParam(View, Dir, sizeof(Dir)) == 0) {
        strlcpy(Dir, Path.c_str(), sizeof(Dir));
        if (View->MView->Win->GetStr("Set directory", sizeof(Dir), Dir, HIST_PATH) == 0)
            return 0;
    }
    if (ExpandPath(Dir, Dir2, sizeof(Dir2)) == -1)
        return 0;
#if 0
    // is this needed for other systems as well ?
    Slash(Dir2, 1);
#endif
    Path = Dir2;
    Row = -1;
    UpdateTitle();
    return RescanDir();
}

int EDirectory::GetContext() { return CONTEXT_DIRECTORY; }
char *EDirectory::FormatLine(int /*Line*/) { return 0; };
int EDirectory::CanActivate(int /*Line*/) { return 1; }
#endif
