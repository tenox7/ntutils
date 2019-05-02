/*    c_desktop.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_desktop.h"

#ifdef CONFIG_DESKTOP

#include "c_commands.h"
#include "e_cvslog.h"
#include "e_mark.h"
#include "e_tags.h"
#include "o_directory.h"
#include "s_util.h"

#include <ctype.h>

#define DESKTOP_VER "FTE Desktop 2\n"
#define DESKTOP_VER1 "FTE Desktop 1\n"

char DesktopFileName[256] = "";

int SaveDesktop(const char *FileName) {
    FILE *fp;
    EModel *M;

    if (!(fp = fopen(FileName, "w")))
        return 0;

    if (setvbuf(fp, FileBuffer, _IOFBF, sizeof(FileBuffer) != 0))
        goto err;

    if (fprintf(fp, DESKTOP_VER) < 0)
        goto err;

    M = ActiveModel;
    while (M) {
        switch(M->GetContext()) {
        case CONTEXT_FILE:
#ifdef CONFIG_OBJ_CVS
            if (M != CvsLogView) {
#else
            {
#endif
                EBuffer *B = (EBuffer *)M;
                if (fprintf(fp, "F|%d|%s\n", B->ModelNo, B->FileName) < 0)
                    goto err;
            }
            break;
#ifdef CONFIG_OBJ_DIRECTORY
        case CONTEXT_DIRECTORY:
            {
                EDirectory *D = (EDirectory *)M;
                if (fprintf(fp, "D|%d|%s\n", D->ModelNo, D->Path.c_str()) < 0)
                    goto err;
            }
            break;
#endif
        }
        M = M->Next;
        if (M == ActiveModel)
            break;
    }
#ifdef CONFIG_TAGS
    TagsSave(fp);
#endif
    markIndex.saveToDesktop(fp);
    return (fclose(fp) == 0) ? 1 : 0;
err:
    fclose(fp);
    return 0;
}

int LoadDesktop(const char *FileName) {
    FILE *fp;
    char line[512];
    char *p, *e;
    int FLCount = 0;

#ifdef CONFIG_TAGS
    TagClear();
#endif

    if (!(fp = fopen(FileName, "r")))
        return 0;

    //setvbuf(fp, FileBuffer, _IOFBF, sizeof(FileBuffer));

    if (!fgets(line, sizeof(line), fp)
        || (strcmp(line, DESKTOP_VER) != 0 &&
            (strcmp(line, DESKTOP_VER1) != 0))) {
        fclose(fp);
        return 0;
    }

    while (fgets(line, sizeof(line), fp) != 0) {
        e = strchr(line, '\n');
        if (e == 0)
            break;
        *e = 0;
        if ((line[0] == 'D' || line[0] == 'F') && line[1] == '|') {
            int ModelNo = -1;
            p = line + 2;
            if (isdigit(*p)) {
                ModelNo = atoi(p);
                while (isdigit(*p)) p++;
                if (*p == '|')
                    p++;
            }

            if (line[0] == 'F') { // file
                if (FLCount > 0)
                    suspendLoads = 1;
                if (FileLoad(0, p, 0, ActiveView))
                    FLCount++;
                suspendLoads  = 0;
#ifdef CONFIG_OBJ_DIRECTORY
            } else if (line[0] == 'D') { // directory
                EModel *m = new EDirectory(0, &ActiveModel, p);
                assert(ActiveModel != 0 && m != 0);
#endif
            }

            if (ActiveModel) {
                if (ModelNo != -1) {
                    if (FindModelID(ActiveModel, ModelNo) == 0)
                        ActiveModel->ModelNo = ModelNo;
                }

                if (ActiveModel != ActiveModel->Next) {
                    suspendLoads = 1;
                    ActiveView->SelectModel(ActiveModel->Next);
                    suspendLoads  = 0;
                }
            }
        } else
#ifdef CONFIG_TAGS
            if (line[0] == 'T' && line[1] == '|') // tag file
                TagsAdd(line + 2);
            else
#endif
                if (line[0] == 'M' && line[1] == '|') { // mark
                char *name;
                char *file;
                EPoint P;
                //long l;
                char *c;

                p = line + 2;
                P.Row = (int)strtol(p, &c, 10);
                if (*c != '|')
                    break;
                p = c + 1;
                P.Col = (int)strtol(p, &c, 10);
                if (*c != '|')
                    break;
                p = c + 1;
                name = p;
                while (*p && *p != '|')
                    p++;
                if (*p == '|')
                    *p++ = 0;
                else
                    break;
                file = p;

                markIndex.insert(name, file, P);
        }
    }
    return (fclose(fp) == 0) ? 1 : 0;
}

#endif
