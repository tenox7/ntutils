/*    commands.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "o_model.h"
#include "s_files.h"
#include "sysdep.h"

int GetDefaultDirectory(EModel *M, char *Path, size_t MaxLen) {
    if (M) 
        M->GetPath(Path, MaxLen);
    if (!M || Path[0] == 0)
        if (ExpandPath(".", Path, MaxLen) == -1)
            return 0;
    SlashDir(Path);
    return 1;
}

int SetDefaultDirectory(EModel *M) {
    char Path[MAXPATH];
    
    if (GetDefaultDirectory(M, Path, sizeof(Path)) == 0)
        return 0;
    if (ChangeDir(Path) == -1)
        return 0;
    return 1;
}
