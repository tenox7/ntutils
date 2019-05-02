/*    o_messages.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "o_messages.h"

#ifdef CONFIG_OBJ_MESSAGES

#include "c_commands.h"
#include "c_config.h"
#include "i_view.h"
#include "o_buflist.h"
#include "s_files.h"
#include "s_string.h"
#include "s_util.h"

#include <stdio.h>

#define MAXREGEXP  32

EMessages *CompilerMsgs = 0;

struct Error {
    EBuffer *Buf;
    StlString file;
    StlString msg;
    StlString text;
    int line;
    int hilit;

    Error(const char *fn, int ln, const char *mesg, const char *txt, int hlt) :
	Buf(0), file(fn), msg(mesg), text(txt), line(ln), hilit(hlt)
    {}
};

static int NCRegexp = 0;
static struct {
    int RefFile;
    int RefLine;
    int RefMsg;
    RxNode *rx;
} CRegexp[MAXREGEXP];

int AddCRegexp(int file, int line, int msg, const char *regexp) {
    //fprintf(stderr, "ADD EXP %d: %s   %d %d %d\n", NCRegexp, regexp, file, line, msg);
    if (NCRegexp >= MAXREGEXP) return 0;
    CRegexp[NCRegexp].RefFile = file;
    CRegexp[NCRegexp].RefLine = line;
    CRegexp[NCRegexp].RefMsg = msg;
    if ((CRegexp[NCRegexp].rx = RxCompile(regexp)) == NULL) {
        return 0;
    }
    NCRegexp++;
    return 1;
}

void FreeCRegexp()
{
    while (NCRegexp--)
        RxFree(CRegexp[NCRegexp].rx);
}

EMessages::EMessages(int createFlags, EModel **ARoot, const char *ADir, const char *ACommand) :
    EList(createFlags, ARoot, "Messages"),
    Running(1),
    BufLen(0),
    BufPos(0),
    ReturnCode(-1),
    MatchCount(0)
{
    CompilerMsgs = this;
    RunPipe(ADir, ACommand);
}

EMessages::~EMessages() {
    gui->ClosePipe(PipeId);
    FreeErrors();
    CompilerMsgs = 0;
}

void EMessages::NotifyDelete(EModel *Deleting) {
    vector_iterate(Error*, ErrList, it) {
        if ((*it)->Buf == Deleting) {
            /* NOT NEEDED!
             char bk[16];
             sprintf(bk, "_MSG.%d", i);
             ((EBuffer *)Deleting)->RemoveBookmark(bk);
             */
            (*it)->Buf = 0;
        }
    }
}

void EMessages::FindErrorFiles() {
    unsigned i = 0;
    vector_iterate(Error*, ErrList, it) {
        if ((*it)->Buf == 0 && (*it)->file.empty())
	    FindErrorFile(i);
        ++i;
    }
}

void EMessages::FindErrorFile(unsigned err) {
    assert(err < ErrList.size());
    if (ErrList[err]->file.empty())
        return;

    ErrList[err]->Buf = 0;

    EBuffer *B = FindFile(ErrList[err]->file.c_str());
    if (B == 0)
        return;

    if (B->Loaded == 0)
        return;

    AddFileError(B, err);
}

void EMessages::AddFileError(EBuffer *B, unsigned err) {

    assert(err < ErrList.size());

    char bk[16];
    sprintf(bk, "_MSG.%d", err);
    EPoint P(ErrList[err]->line - 1, 0);

    if (P.Row >= B->RCount)
        P.Row = B->RCount - 1;
    if (P.Row < 0)
        P.Row = 0;

    if (B->PlaceBookmark(bk, P) == 1)
        ErrList[err]->Buf = B;
}

void EMessages::FindFileErrors(EBuffer *B) {
    unsigned i = 0;
    vector_iterate(Error*, ErrList, it) {
        if ((*it)->Buf == 0 && !(*it)->file.empty()) {
            if (filecmp(B->FileName, (*it)->file.c_str()) == 0)
                AddFileError(B, i);
	}
	++i;
    }
}

int EMessages::RunPipe(const char *ADir, const char *ACommand) {
    if (!KeepMessages)
        FreeErrors();
    
    Command = ACommand;
    Directory = ADir;
    
    MatchCount = 0;
    ReturnCode = -1;
    Running = 1;
    BufLen = BufPos = 0;
    Row = (int)ErrList.size() - 1;

    char s[2 * MAXPATH * 4];

    sprintf(s, "[running '%s' in '%s']", ACommand, ADir);
    AddError(0, -1, 0, s);

    sprintf(s, "Messages [%s]: %s", ADir, ACommand);
    SetTitle(s);
    
    ChangeDir(ADir);
    PipeId = gui->OpenPipe(ACommand, this);
    return 0;
}

EEventMap *EMessages::GetEventMap() {
    return FindEventMap("MESSAGES");
}

int EMessages::ExecCommand(ExCommands Command, ExState &State) {
    switch (Command) {
    case ExChildClose:
        if (Running == 0 || PipeId == -1)
            break;
        ReturnCode = gui->ClosePipe(PipeId);
        PipeId = -1;
        Running = 0;
        char s[30];
        sprintf(s, "[aborted, status=%d]", ReturnCode);
        AddError(0, -1, 0, s);
        return 1;
    case ExActivateInOtherWindow:
        ShowError(View->Next, Row);
        return 1;
    case ExFind:
        fprintf(stderr, "FIND\n");
        return 1;
    default:
        ;
    }
    return EList::ExecCommand(Command, State);
}

void EMessages::AddError(const char *file, int line, const char *msg, const char *text, int hilit) {
    ErrList.push_back(new Error(file, line, msg, text, hilit));
    //fprintf(stderr, "Error %s %d %s %s\n", file, line, msg, text);
    FindErrorFile((unsigned)ErrList.size() - 1);

    if ((int)ErrList.size() > Count)
        if (Row >= Count - 1)
	    Row = (int)ErrList.size() - 1;

    UpdateList();
}

void EMessages::FreeErrors() {
    unsigned i = 0;
    vector_iterate(Error*, ErrList, it) {
	if ((*it)->Buf != 0) {
	    char bk[16];
	    sprintf(bk, "_MSG.%d", i);
	    (*it)->Buf->RemoveBookmark(bk);
	}
	delete *it;
        i++;
    }
    ErrList.clear();
    BufLen = BufPos = 0;
}

int EMessages::GetLine(char *Line, size_t maxim) {
    ssize_t rc;
    char *p;
    int l;
    
    //fprintf(stderr, "GetLine: %d\n", Running);

    *Line = 0;
    if (Running && PipeId != -1) {
        rc = gui->ReadPipe(PipeId, MsgBuf + BufLen, sizeof(MsgBuf) - BufLen);
        //fprintf(stderr, "GetLine: ReadPipe rc = %d\n", rc);
        if (rc == -1) {
            ReturnCode = gui->ClosePipe(PipeId);
            PipeId = -1;
            Running = 0;
        }
        if (rc > 0)
            BufLen += rc;
    }
    l = maxim - 1;
    if (BufLen - BufPos < l)
        l = BufLen - BufPos;
    //fprintf(stderr, "GetLine: Data %d\n", l);
    p = (char *)memchr(MsgBuf + BufPos, '\n', l);
    if (p) {
        *p = 0;
        UnEscStr(Line, maxim, MsgBuf + BufPos, p - (MsgBuf + BufPos));
        //strcpy(Line, MsgBuf + BufPos);
        l = strlen(Line);
        if (l > 0 && Line[l - 1] == '\r')
            Line[l - 1] = 0;
        BufPos = p + 1 - MsgBuf;
        //fprintf(stderr, "GetLine: Line %d\n", strlen(Line));
    } else if (Running && sizeof(MsgBuf) != BufLen) {
        memmove(MsgBuf, MsgBuf + BufPos, BufLen - BufPos);
        BufLen -= BufPos;
        BufPos = 0;
        //fprintf(stderr, "GetLine: Line Incomplete\n");
        return 0;
    } else {
        if (l == 0) 
            return 0;
        UnEscStr(Line, maxim, MsgBuf + BufPos, l);
        //memcpy(Line, MsgBuf + BufPos, l);
        Line[l] = 0;
        if (l > 0 && Line[l - 1] == '\r')
            Line[l - 1] = 0;
        BufPos += l;
        //fprintf(stderr, "GetLine: Line Last %d\n", l);
    }
    memmove(MsgBuf, MsgBuf + BufPos, BufLen - BufPos);
    BufLen -= BufPos;
    BufPos = 0;
    //fprintf(stderr, "GetLine: Got Line\n");
    return 1;
}


static void getWord(char* dest, const char* pin)
{
    char *pout, *pend;
    char ch, ec;
    
    while (*pin == ' ' || *pin == '\t')
        pin++;

    pout = dest;
    pend = dest + 256 - 1;
    if (*pin == '\'' || *pin == '"' || *pin == '`') {
        ec = *pin++;
        if (ec == '`')
            ec = '\'';
        for (;;) {
            ch  = *pin++;
            if (ch == '`')
                ch = '\'';
            if (ch == ec || ch == 0)
                break;
            
            if (pout < pend)
                *pout++ = ch;
        }
        if (ch == 0)
            pin--;
    } else {
        for(;;) {
            ch  = *pin++;
            if (ch == ' ' || ch == '\t' || ch == 0)
                break;
            if (pout < pend) *pout++ = ch;
        }
    }
    *pout = 0;
}


void EMessages::GetErrors() {
    char line[4096];
    RxMatchRes RM;
    //int retc;
    int i, n;
    int didmatch = 0;
    int WasRunning = Running;
    char fn[256];
    
    //fprintf(stderr, "Reading pipe\n");
    while (GetLine(line, sizeof(line))) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
	    line[--len] = 0;
        didmatch = 0;
        for (i = 0; i < NCRegexp; i++) {
            if (RxExec(CRegexp[i].rx, line, len, line, &RM) == 1) {
		char ln[256];
		char msg[256];
		char fn1[256];
		char fn2[256];
		char *file;

		n = CRegexp[i].RefFile;
		unsigned s = RM.Close[n] - RM.Open[n];
		if (s < sizeof(fn))
		    memcpy(fn, line + RM.Open[n], s);
		else
		    s = 0;
		fn[s] = 0;

		n = CRegexp[i].RefLine;
		s = RM.Close[n] - RM.Open[n];
		if (s < sizeof(ln))
		    memcpy(ln, line + RM.Open[n], s);
		else
		    s = 0;
		ln[s] = 0;

		n = CRegexp[i].RefMsg;
		s = RM.Close[n] - RM.Open[n];
		if (s < sizeof(msg))
		    memcpy(msg, line + RM.Open[n], s);
		else
		    s = 0;
		msg[s] = 0;
		//fprintf(stderr, "File:%s msg:%s rex:%d c:%d o:%d>%s<8\nTXT:%s\n", fn, ln, i, RM.Close[n], RM.Open[n], msg, line);
		if (IsFullPath(fn))
		    file = fn;
		else {
		    /*
		     * for now - try only with top most dir
		     * later we might try to find the file in all stacked dirs
		     * as with parallel makes it's hard to guess the right directory path
                     */
		    strlcpy(fn1, DirLevel.size() ? DirLevel.back().c_str() : Directory.c_str(),
			    sizeof(fn1));
                    Slash(fn1, 1);
                    strlcat(fn1, fn, sizeof(fn1));
                    if (ExpandPath(fn1, fn2, sizeof(fn2)) == 0)
                        file = fn2;
                    else
                        file = fn1;
                }
                AddError(file, atoi(ln), msg, line, 1);
                didmatch = 1;
                MatchCount++;
                break;
            }
        }
        if (!didmatch)
        {
            AddError(0, -1, 0, line);
            //** Quicky: check for gnumake 'entering directory'
            //** make[x]: entering directory `xxx'
            //** make[x]: leaving...
            static const char t1[] = "entering directory";
            static const char t2[] = "leaving directory";
            const char *pin;
            
            if ( (pin = strstr(line, "]:")) != 0)
            {
                //** It *is* some make line.. Check for 'entering'..
                pin += 2;
                while (*pin == ' ')
                    pin++;
                if (strnicmp(pin, t1, sizeof(t1)-1) == 0) {  // Entering?
                    //** Get the directory name from the line,
                    pin += sizeof(t1)-1;
                    getWord(fn, pin);
                    //dbg("entering %s", fn);

		    if (*fn)
                        //** Indeed entering directory! Link in list,
			DirLevel.push_back(fn);
                } else if (strnicmp(pin, t2, sizeof(t2)-1) == 0) {  // Leaving?
                    pin += sizeof(t2)-1;
                    getWord(fn, pin);                   // Get dirname,
                    //dbg("leaving %s", fn);
		    int found = 0;
		    for (unsigned i = DirLevel.size(); i-- > 0;) {
			/*
			 * remove leaved director from our list of Dirs
			 * as many users runs make in parallel mode
			 * we might get pretty mangled order of dirs
                         * so remove the last added with the same name
                         */
			if (stricmp(DirLevel[i].c_str(), fn) == 0) {
			    DirLevel.erase(DirLevel.begin() + i);
                            found++;
			    break;
			}
		    }
		    if (!found) {
                        //** Mismatch filenames -> error, and revoke stack.
                        //dbg("mismatch on %s", fn);
                        AddError(0, -1, 0, "fte: mismatch in directory stack!?");
			//** In this case we totally die the stack..
                        DirLevel.clear();
                    }
                }
            }
        }
    }
    //fprintf(stderr, "Reading Stopped\n");
    if (!Running && WasRunning) {
        char s[30];
        
        sprintf(s, "[done, status=%d]", ReturnCode);
        AddError(0, -1, 0, s);
    }
    //UpdateList();
    //NeedsUpdate = 1;
}

int EMessages::CompilePrevError(EView *V) {
    if (!ErrList.size()) {
        V->Msg(S_INFO, "No errors.");
        return 0;
    }

    while (Row > 0) {
        Row--;
        if (ErrList[Row]->line != -1 && !ErrList[Row]->file.empty()) {
            ShowError(V, Row);
            return 1;
        }
    }

    V->Msg(S_INFO, "No previous error.");
    return 0;
}

int EMessages::CompileNextError(EView *V) {
    if (!ErrList.size()) {
        V->Msg(S_INFO, (Running) ? "No errors (yet)." : "No errors.");
        return 0;
    }

    while ((Row + 1) < (int)ErrList.size()) {
        Row++;
        if (ErrList[Row]->line != -1 && !ErrList[Row]->file.empty()) {
            ShowError(V, Row);
            return 1;
        }
    }

    V->Msg(S_INFO, (Running) ? "No more errors (yet)." : "No more errors.");
    return 0;
}

int EMessages::Compile(char * /*Command*/) {
    return 0;
}

void EMessages::ShowError(EView *V, unsigned err) {
    if (err < ErrList.size()) {
        if (!ErrList[err]->file.empty()) {
            // should check if relative path
            // possibly scan for (gnumake) directory info in output
            if (ErrList[err]->Buf) {
                char bk[16];

                V->SwitchToModel(ErrList[err]->Buf);

                sprintf(bk, "_MSG.%d", err);
                ErrList[err]->Buf->GotoBookmark(bk);
            } else {
                if (FileLoad(0, ErrList[err]->file.c_str(), 0, V) == 1) {
                    V->SwitchToModel(ActiveModel);
                    ((EBuffer *)ActiveModel)->CenterNearPosR(0, ErrList[err]->line - 1);
                }
            }
            if (!ErrList[err]->msg.empty())
                V->Msg(S_INFO, "%s", ErrList[err]->msg.c_str());
            else
                V->Msg(S_INFO, "%s", ErrList[err]->text.c_str());
        }
    }
}

void EMessages::DrawLine(PCell B, int Line, int Col, ChColor color, int Width) {
    if (Line < (int)ErrList.size())
        if (Col < int(ErrList[Line]->text.size())) {
            char str[1024];
            size_t len;

            len = UnTabStr(str, sizeof(str),
                           ErrList[Line]->text.c_str(),
                           ErrList[Line]->text.size());

            if ((int)len > Col)
                MoveStr(B, 0, Width, str + Col, color, Width);
        }
}

char* EMessages::FormatLine(int Line) {
    if (Line < (int)ErrList.size())
        return strdup(ErrList[Line]->text.c_str());

    return 0;
}

int EMessages::IsHilited(int Line) {
    return (Line >= 0 && Line < (int)ErrList.size()) ? ErrList[Line]->hilit : 0;
}

void EMessages::UpdateList() {
    Count = (int)ErrList.size();
    EList::UpdateList();
}

int EMessages::Activate(int /*No*/) {
    //assert(No == Row);
    //Row = No;
    ShowError(View, Row);
    return 1;
}

int EMessages::CanActivate(int Line) {
    //return (Line < (int)ErrList.size());
    return (Line < (int)ErrList.size()
            && (!ErrList[Line]->file.empty()
                || ErrList[Line]->line != -1)) ? 1 : 0;
}
    
void EMessages::NotifyPipe(int APipeId) {
    //fprintf(stderr, "Got notified");
    if (APipeId == PipeId)
        GetErrors();
}

void EMessages::GetName(char *AName, size_t MaxLen) {
    strlcpy(AName, "Messages", MaxLen);
}

void EMessages::GetInfo(char *AInfo, size_t /*MaxLen*/) {
    sprintf(AInfo, "%2d %04d/%03d Messages: %d (%s)",
            ModelNo,Row, Count, MatchCount, Command.c_str());
}

void EMessages::GetPath(char *APath, size_t MaxLen) {
    strlcpy(APath, Directory.c_str(), MaxLen);
    Slash(APath, 0);
}

void EMessages::GetTitle(char *ATitle, size_t MaxLen, char *ASTitle, size_t SMaxLen) {
    snprintf(ATitle, MaxLen, "Messages: %s", Command.c_str());
    strlcpy(ASTitle, "Messages", SMaxLen);
}

// get row length for specified row, used in MoveLineEnd to get actual row length
size_t EMessages::GetRowLength(int ARow)
{
    if ((ARow >= 0) && (ARow < (int)ErrList.size()))
        return ErrList[ARow]->text.size();

    return 0;
}

#endif // CONFIG_OBJ_MESSAGES
