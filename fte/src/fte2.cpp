/*    fte.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "fte.h"

#if defined(UNIX)
/* Some paths where to also look for the rc-file */
/* How many */
#define UNIX_RCPATHS 3

/* Actual locations */
static const char * const Unix_RCPaths[UNIX_RCPATHS]={
    "/usr/local/etc/fte/system.fterc",
    "/etc/fte/system.fterc",
    "/usr/X11R6/lib/X11/xfte/system.fterc",
};

#endif /* UNIX */

static void Usage() {
    printf("Usage: " PROGRAM " [-!] [-?] [--help] [-h] [-crdnm] files...\n"
           "Version: " VERSION " " COPYRIGHT "\n"
           "   You may distribute under the terms of either the GNU General Public\n"
           "   License or the Artistic License, as specified in the README file.\n"
           "\n"
           "Options:\n"
           "   --                End of options, only files remain.\n"
           "   -+                Next option is file.\n"
           "   -?                Display usage.\n"
           "   -h                Display usage.\n"
           "   --help            Display usage.\n"
           "   -!                Ignore config file, use builtin defaults (also -c).\n"
           "   -c[<.cnf>]        Use specified configuration file (no arg=builtin).\n"
#ifdef CONFIG_DESKTOP
           "   -d[<.dsk>]        Load/Save desktop from <.dsk> (no arg=disable desktop).\n"
#endif
/*
#ifdef CONFIG_HISTORY
           "   -h[<.his>]        Load/Save history from <.his> (no arg=disable history).\n"
#endif
*/
           "   -m[<mode>]        Override mode for remaining files (no arg=no override).\n"
           "   -l<line>[,<col>]  Go to line (and column) in next file.\n"
           "   -r                Open next file as read-only.\n"
#ifdef CONFIG_TAGS
           "   -T[<tagfile>]     Load tags file at startup.\n"
           "   -t<tag>           Locate specified tag.\n"
#endif
//           "       -p        Load files into already running FTE.\n"
        );
}

/*
 * findPathExt() returns a ^ to the suffix in a file name string. If the
 * name contains a suffix, the pointer ^ts to the suffix' dot character,
 * if the name has no suffix the pointer points to the NUL terminator of
 * the file name string.
 * .lib: CBASE.LIB
 */
static char *findPathExt(char *filename) {
    char *p, *sps;

    for (p = filename, sps = NULL; *p; p++) {
        if (ISSLASH(*p))
            sps = NULL;
        if (*p == '.')
            sps = p;
    }
    if (sps == NULL)
        sps = p;
    return sps;
}

#if defined(OS2) && defined(__EMX__)

// argv[0] on emx does not contain full path

#define INCL_DOS
#include <os2.h>

char *getProgramName(char *name) {
    char ProgramName[MAXPATH];
    PTIB tib;
    PPIB pib;

    DosGetInfoBlocks(&tib, &pib);
    if (DosQueryModuleName(pib->pib_hmte, sizeof(ProgramName), ProgramName) != 0)
        return name;
    return strdup(ProgramName);
}

#endif

static int GetConfigFileName(int argc, char **argv, char *ConfigFileName) {
    int i;
    char CfgName[MAXPATH] = "";
    
    if (ConfigFileName[0] == 0) {
#if defined(UNIX)
        // ? use ./.fterc if by current user ?
        ExpandPath("~/.fterc", CfgName);
#elif defined(DOS) || defined(DOSP32)
        strcpy(CfgName, argv[0]);
        strcpy(findPathExt(CfgName), ".cnf");
#elif defined(OS2) || defined(NT)
        char home[MAXPATH] = "";
        char *ph;
#if defined(OS2)
        ph = getenv("HOME");
        if (ph) strcpy(home, ph);
#endif
#if defined(NT)
        ph = getenv("HOMEDRIVE");
        if (ph) strcpy(home, ph);
        ph = getenv("HOMEPATH");
        if (ph) strcat(home, ph);
#endif
        if (home[0]) {
            strcpy(CfgName, home);
            Slash(CfgName, 1);
            strcat(CfgName, "fte.cnf");
        }

        if (!home[0] || access(CfgName, 0) != 0) {
            strcpy(CfgName, argv[0]);
            strcpy(findPathExt(CfgName), ".cnf");
        }
#endif

        strcpy(ConfigFileName, CfgName);
    }
//    printf("Trying '%s'...",ConfigFileName);
    if (access(ConfigFileName, 0) == 0)
    {
//        printf("success, loading!\n");
        return 1;
    }
//    printf("failed!\n");

#if defined(UNIX_RCPATHS)
    for (i=0;i<UNIX_RCPATHS;i++)
    {
//        printf("Trying '%s'...",Unix_RCPaths[i]);
        if (access(Unix_RCPaths[i],0) == 0)
        {
//            printf("success, loading!\n");
            strcpy(ConfigFileName,Unix_RCPaths[i]);
            return 1;
        }
//        printf("failed!\n");

    }
#endif

//    printf("No config-file found!\n");
    return 0;
}

static int CmdLoadConfiguration(int &argc, char **argv) {
    int ign = 0;
    int QuoteAll = 0, QuoteNext = 0;
    int Arg;
    char ConfigFileName[MAXPATH] = "";

    for (Arg = 1; Arg < argc; Arg++) {
        if (!QuoteAll && !QuoteNext && (argv[Arg][0] == '-')) {
            if (stricmp(argv[Arg],"--help")==0) {
                Usage();
                return 0;
            } else if (argv[Arg][1] == '-') {
                QuoteAll = 1;
            } else if (argv[Arg][1] == '!') {
                ign = 1;
            } else if (argv[Arg][1] == '+') {
                QuoteNext = 1;
            } else if (argv[Arg][1] == 'c') {
                if (argv[Arg][2])
                    strcpy(ConfigFileName, argv[Arg] + 2);
                else
                    ign = 1;
            } else if (argv[Arg][1] == '?') {
                Usage();
                return 0;
            } else if (argv[Arg][1] == 'h') {
                Usage();
                return 0;
            } 
        } 
    }

    if (GetConfigFileName(argc, argv, ConfigFileName) == 0)
        ; // should we default to internal (NOT?)
        
    if (ign) {
        if (UseDefaultConfig() == -1)
            DieError(1, "Error in internal configuration??? FATAL!");
    } else {
        if (LoadConfig(argc, argv, ConfigFileName) == -1)
            DieError(1,
                     "Failed to find any configuration file.\n"
                     "Use '-c' option.");
    }
    for (Arg = 1; Arg < argc; Arg++) {
        if (!QuoteAll && !QuoteNext && (argv[Arg][0] == '-')) {
            if (argv[Arg][1] == '-') {
                QuoteAll = 1;
            } else if (argv[Arg][1] == '+') {
                QuoteNext = 1;
#ifdef CONFIG_DESKTOP
            } else if (argv[Arg][1] == 'd') {
                strcpy(DesktopFileName, argv[Arg] + 2);
                if (DesktopFileName[0] == 0) {
                    LoadDesktopOnEntry = 0;
                    SaveDesktopOnExit = 0;
                } else {
                    LoadDesktopOnEntry = 1;
                }
#endif
#ifdef CONFIG_HISTORY
            } else if (argv[Arg][1] == 'h') {
                strcpy(HistoryFileName, argv[Arg] + 2);
                if (HistoryFileName[0] == 0) {
                    KeepHistory = 0;
                } else {
                    KeepHistory = 1;
                }
#endif
            }
        } 
    }
    return 1;
}

static int CmdLoadFiles(int &argc, char **argv) {
    int QuoteNext = 0;
    int QuoteAll = 0;
    int GotoLine = 0;
    int LineNum = 1;
    int ColNum = 1;
    int ModeOverride = 0;
    char Mode[32];
    int LCount = 0;
    int ReadOnly = 0;
    
    for (int Arg = 1; Arg < argc; Arg++) {
        if (!QuoteAll && !QuoteNext && (argv[Arg][0] == '-')) {
            if (argv[Arg][1] == '-') {
                QuoteAll = 1;
            } else if (argv[Arg][1] == '!') {
                // handled before
            } else if (argv[Arg][1] == 'c') {
                // ^
            } else if (argv[Arg][1] == 'd') {
                // ^
            } else if (argv[Arg][1] == 'h') {
                // ^
            } else if (argv[Arg][1] == '+') {
                QuoteNext = 1;
            } else if (argv[Arg][1] == '#' || argv[Arg][1] == 'l') {
                LineNum = 1;
                ColNum = 1;
                if (strchr(argv[Arg], ',')) {
                    GotoLine = (2 == sscanf(argv[Arg] + 2, "%d,%d", &LineNum, &ColNum));
                } else {
                    GotoLine = (1 == sscanf(argv[Arg] + 2, "%d", &LineNum));
                }
                //                printf("Gotoline = %d, line = %d, col = %d\n", GotoLine, LineNum, ColNum);
            } else if (argv[Arg][1] == 'r') {
                ReadOnly = 1;
            } else if (argv[Arg][1] == 'm') {
               if (argv[Arg][2] == 0) {
                   ModeOverride = 0;
               } else {
                   ModeOverride = 1;
                   strcpy(Mode, argv[Arg] + 2);
               }
            } else if (argv[Arg][1] == 'T') {
                TagsAdd(argv[Arg] + 2);
            } else if (argv[Arg][1] == 't') {
                TagGoto(0, argv[Arg] + 2);
            } else if (argv[Arg][1] == '?') {
                Usage();
                return 0;
            } else {
                DieError(2, "Invalid command line option %s", argv[Arg]);
                return 0;
            }
        } else {
            char Path[MAXPATH];

            if (ExpandPath(argv[Arg], Path) == 0 && IsDirectory(Path)) {
                new EDirectory(&MM, Path);
                assert(MM != 0);
                //VV->SwitchToModel(MM);
            } else {
                if (ModeOverride) {
                    if (MultiFileLoad(argv[Arg], Mode, 0) == 0) return 0;
                } else {
                    if (MultiFileLoad(argv[Arg], 0, 0) == 0) return 0;
                }
                if (((EBuffer *)MM)->Loaded == 0)
                    ((EBuffer *)MM)->Load();
                if (GotoLine) {
                    GotoLine = 0;
                    ((EBuffer *)MM)->SetNearPosR(ColNum - 1, LineNum - 1);
                } else {
                    int r, c;
                    
                    if (RetrieveFPos(((EBuffer *)MM)->FileName, r, c) == 1)
                        ((EBuffer *)MM)->SetNearPosR(c, r);
                }
                
                if (ReadOnly) {
                    ReadOnly = 0;
                    BFI(((EBuffer *)MM), BFI_ReadOnly) = 1;
                }
            }
            QuoteNext = 0;
            MM = MM->Next;
            LCount++;
        }
    }
    while (LCount-- > 0)
        MM = MM->Prev;
    return 1;
}

#ifdef CONFIG_HISTORY
static void DoLoadHistoryOnEntry(int &argc, char **argv) {
    if (HistoryFileName[0] == 0) {
#ifdef UNIX
        ExpandPath("~/.fte-history", HistoryFileName);
#else
        JustDirectory(argv[0], HistoryFileName);
        strcat(HistoryFileName, "fte.his");
#endif
    } else {
        char p[256];
        
        ExpandPath(HistoryFileName, p);
        if (IsDirectory(p)) {
            Slash(p, 1);
#ifdef UNIX
            strcat(p, ".fte-history");
#else
            strcat(p, "fte.his");
#endif
        }
        strcpy(HistoryFileName, p);
    }
    
    if (KeepHistory && FileExists(HistoryFileName))
        LoadHistory(HistoryFileName);
}

static void DoSaveHistoryOnExit() {
    if (KeepHistory && HistoryFileName[0] != 0)
        SaveHistory(HistoryFileName);
}
#endif

#ifdef CONFIG_DESKTOP
void DoLoadDesktopOnEntry(int &argc, char **argv) {
    if (DesktopFileName[0] == 0) {
#ifdef UNIX
        if (FileExists(".fte-desktop")) {
            ExpandPath(".fte-desktop", DesktopFileName);
        } else {
            ExpandPath("~/.fte-desktop", DesktopFileName);
        }
#else
        if (FileExists("fte.dsk")) {
            ExpandPath("fte.dsk", DesktopFileName);
        } else {
            JustDirectory(argv[0], DesktopFileName);
            strcat(DesktopFileName, "fte.dsk");
        }
#endif
    } else {
        char p[MAXPATH];
        
        ExpandPath(DesktopFileName, p);
        if (IsDirectory(p)) {
            Slash(p, 1);
#ifdef UNIX
            strcat(p, ".fte-desktop");
#else
            strcat(p, "fte.dsk");
#endif
        }
        strcpy(DesktopFileName, p);
    }

    if (LoadDesktopOnEntry && FileExists(DesktopFileName))
        LoadDesktop(DesktopFileName);
}
#endif

static void EditorInit() {
    SS = new EBuffer((EModel **)&SS, "Scrap");
    BFI(SS, BFI_Undo) = 0; // disable undo for clipboard
    MM = 0;
}

static void EditorCleanup() {
    EModel *B, *N, *A;
    EView *BW, *NW, *AW;
    
    if (MM) {
        B = A = MM;
        while (B != A) {
            N = B->Next;
            delete B;
            B = N;
        }
    }
    MM = 0;

    delete SS;
    SS = 0;
    
    if (VV) {
        BW = AW = VV;
        while (BW != AW) {
            NW = BW->Next;
            delete BW;
            BW = NW;
        }
    }
    VV = 0;
}

static int InterfaceInit(int &argc, char **argv) {
    GxView *view;
    ExModelView *edit;
    
    new EGUI(argc, argv, ScreenSizeX, ScreenSizeY);
    if (gui == 0)
        DieError(1, "Failed to initialize display\n");
    
    new EFrame(ScreenSizeX, ScreenSizeY);
    if (frames == 0)
        DieError(1, "Failed to create window\n");
    
    //frames->SetMenu("Main"); //??
    
    view = new GxView(frames);
    if (view == 0)
        DieError(1, "Failed to create view\n");

    VV = new EView(MM);
    assert(VV != 0);

    edit = new ExModelView(VV);
    if (!edit)
	return 0;
    view->PushView(edit);
    return 1;
}

static void InterfaceCleanup() {
    while (frames)
        delete frames;

    delete gui;
}

int main(int argc, char **argv) {
#if defined(__EMX__)
    argv[0] = getProgramName(argv[0]);
#endif
    
    if (CmdLoadConfiguration(argc, argv) == 0)
        return 1;

#ifdef CONFIG_HISTORY
    DoLoadHistoryOnEntry(argc, argv);
#endif

    EditorInit();

#ifdef CONFIG_DESKTOP
    DoLoadDesktopOnEntry(argc, argv);
#endif

    if (CmdLoadFiles(argc, argv) == 0)
        return 3;

    if (MM == 0) {
#ifdef CONFIG_OBJ_DIRECTORY
        char Path[MAXPATH];
        
        GetDefaultDirectory(0, Path, sizeof(Path));
        MM = new EDirectory(&MM, Path);
        assert(MM != 0);
        //VV->SwitchToModel(MM);
#else
        Usage();
        return 1;
#endif
    }
    if (!InterfaceInit(argc, argv))
        return 2;

    gui->Run(); // here starts the PM second thread, so the above blocks the SIQ ;-(

#ifdef CONFIG_HISTORY
    DoSaveHistoryOnExit();
#endif
    
    EditorCleanup();

    InterfaceCleanup();
    
#if defined(OS2)
    if (_heapchk() != _HEAPOK)
        DieError(0, "Heap memory is corrupt.");
#endif
    return 0;
}
