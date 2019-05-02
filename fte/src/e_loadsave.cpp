/*    e_loadsave.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "e_cvslog.h"
#include "e_svnlog.h"
#include "i_modelview.h"
#include "i_view.h"
#include "s_util.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

int EBuffer::Load() {
    return LoadFrom(FileName);
}

int EBuffer::Reload() {
    int R = VToR(CP.Row), C = CP.Col;

    if (!LoadFrom(FileName))
        return 0;
    SetNearPosR(C, R);
    return 1;
}

int EBuffer::Save() {
    if (BFI(this, BFI_ReadOnly)) {
        Msg(S_ERROR, "File is read-only.");
        return 0;
    }
    if (BFI(this, BFI_TrimOnSave))
        FileTrim();
    return SaveTo(FileName);
}

char FileBuffer[RWBUFSIZE];

int EBuffer::LoadFrom(const char *AFileName) {
    int fd;
    ssize_t len = 0, partLen;
    unsigned long numChars = 0, Lines = 0;
    char *p, *e, *m = NULL;
    int lm = 0;
    int lf;
    int SaveUndo, SaveReadOnly;
    int first = 1;
    int strip = BFI(this, BFI_StripChar);
    int lchar = BFI(this, BFI_LineChar);
    int margin = BFI(this, BFI_LoadMargin);

    FileOk = 0;
    fd = open(AFileName, O_RDONLY | O_BINARY, 0);
    if (fd == -1) {
        if (errno != ENOENT) {
            Msg(S_INFO, "Could not open file %s (errno=%d, %s).",
                AFileName, errno, strerror(errno));
        } else {
            Msg(S_INFO, "New file %s.", AFileName);
        }
        Loaded = 1;
        return 0;
    }
    Loading = 1;
    Clear();
    BlockUnmark();
    SaveUndo = BFI(this, BFI_Undo);
    SaveReadOnly = BFI(this, BFI_ReadOnly);
    BFI(this, BFI_Undo) = 0;
    BFI(this, BFI_ReadOnly) = 0;

    while ((len = read(fd, FileBuffer, sizeof(FileBuffer))) > 0) {
        if (first) {
            first = 0;
            if (BFI(this, BFI_DetectLineSep)) {
                int was_lf = 0, was_cr = 0;
                for (int c = 0; c < len; c++) {
                    if (FileBuffer[c] == '\n') {
                        was_lf++;
                        break;
                    } else if (FileBuffer[c] == '\r') {
                        was_cr++;
                        if (was_cr == 2)
                            break;
                    }
                }
                /* !!! fails if first line > 32k
                 * ??? set first to 1 in that case ? */
                if (was_cr || was_lf) {
                    BFI(this, BFI_StripChar) = -1;
                    BFI(this, BFI_LoadMargin) = -1;
                    BFI(this, BFI_AddLF) = 0;
                    BFI(this, BFI_AddCR) = 0;
                    if (was_lf) {
                        BFI(this, BFI_LineChar) = '\n';
                        BFI(this, BFI_AddLF) = 1;
                        if (was_cr) {
                            BFI(this, BFI_StripChar) = '\r';
                            BFI(this, BFI_AddCR) = 1;
                        }
                    } else if (was_cr) {
                        BFI(this, BFI_LineChar) = '\r';
                        BFI(this, BFI_AddCR) = 1;
                    } else {
                        BFI(this, BFI_LineChar) = -1;
                        BFI(this, BFI_LoadMargin) = 64;
                    }
                    strip = BFI(this, BFI_StripChar);
                    lchar = BFI(this, BFI_LineChar);
                    margin = BFI(this, BFI_LoadMargin);
                }
            }
        }
        p = FileBuffer;
        do {
            if (lchar != -1) {     // do we have a LINE delimiter
                e = (char *)memchr((void *)p, lchar, FileBuffer - p + len);
                if (e == NULL) {
                    e = FileBuffer + len;
                    lf = 0;
                } else
                    lf = 1;
            } else if (margin != -1) { // do we have a right margin for wrap
                if (FileBuffer + len >= p + margin) {
                    e = p + margin;
                    lf = 1;
                } else {
                    e = FileBuffer + len;
                    lf = 0;
                }
            } else {
                e = FileBuffer + len;
                lf = 0;
            }
	    partLen = e - p; // # of chars in buffer for current line
            m = (char *)realloc((void *)m, (lm + partLen) + CHAR_TRESHOLD);
            if (m == NULL)
                goto fail;
            memcpy(m + lm, p, partLen);
            lm += partLen;
            numChars += partLen;

            if (lf) {
                // there is a new line, add it to buffer

                if (lm == 0 && !m && !(m = (char *)malloc(CHAR_TRESHOLD)))
                    goto fail;
#if 0
                { // support for VIM tabsize commands
                    char *t = strstr(m,"vi:ts=");
                    int ts = 0;
                    if (t && isdigit(t[6]))
                        ts = atoi(&t[6]);
                    if (ts > 0 && ts <= 16)
                        BFI(this, BFI_TabSize) = ts;
                }
#endif
                // Grow the line table if required,
                if (RCount == RAllocated)
                    if (!Allocate(RCount ? (RCount * 2) : 1))
                        goto fail;
                if (!(LL[RCount++] = new ELine(m, lm)))
                    goto fail;
                RGap = RCount;

                lm = 0;
                m = NULL;
                Lines++;
            }

            p = e;
            if (lchar != -1) // skip LINE terminator/separator
                p++;
        } while (lf);
        Msg(S_INFO, "Loading: %d lines, %d bytes.", Lines, numChars);
    }

    if ((RCount == 0) || (lm > 0) || !BFI(this, BFI_ForceNewLine)) {
        if (lm == 0 && m == NULL && (m = (char *)malloc(CHAR_TRESHOLD)) == 0)
            goto fail;
        // Grow the line table if required,
        if (RCount == RAllocated)
            if (Allocate(RCount ? (RCount * 2) : 1) == 0)
                goto fail;
        if ((LL[RCount++] = new ELine(m, lm)) == 0)
            goto fail;
        m = NULL;
        RGap = RCount;
    }

    // Next time when you introduce something like this
    // check all code paths - as the whole memory management
    // is broken - you have forget to clear 'm' two line above comment!
    // kabi@users.sf.net
    // this bug has caused serious text corruption which is the worst
    // thing for text editor
    if (m)
	free(m);
    m = NULL;

    // initialize folding array.
    // TODO: allocate only when folds are actually used.
    // needs fixing a lot of code in other places.
    VCount = RCount;
    VGap = VCount;
    if (AllocVis(VCount ? VCount : 1) == 0)
        goto fail;
    memset(VV, 0, VCount * sizeof(int));

    if (strip != -1) { // strip CR character from EOL if specified

        // TODO: this should be done during load above to improve performance
        for (int l = 0; l < RCount; l++) {
            if (LL[l]->Count > 0)
                if (LL[l]->Chars[LL[l]->Count - 1] == strip)
                    LL[l]->Count--;
        }
    }
    if ((BFI(this, BFI_SaveFolds) != 0) || BFI(this, BFI_SaveBookmarks) == 1 || BFI(this, BFI_SaveBookmarks) == 2) {
        int len_start = 0, len_end = 0;
        int level = 0, open = 0;
        int l;
        int pos = -1, startpos;
        char foldnum[3] = "00";

        if (BFS(this, BFS_CommentStart) == 0) len_start = 0;
        else len_start = strlen(BFS(this, BFS_CommentStart));
        if (BFS(this, BFS_CommentEnd) == 0) len_end = 0;
        else len_end = strlen(BFS(this, BFS_CommentEnd));

        // How folds and bookmarks are saved in line comment:
        //
        // CommentStart((FOLD)|(fold)nn)?(BOOK(bookmark)+xllllb)?CommentEnd
        //
        // After comment start there can be a fold - open or closed - with level number (nn - decimal).
        // Then bookmarks can follow marked by BOOK "keyword". Last bookmark is terminated by 'x'
        // character followed by total length of bookmark info (llll - hex) and character 'b'. The
        // total length is needed when looking for comment start from the line end.
        // Each bookmark has format "ccccllname" - cccc (hex) is bookmark column, ll (hex) is length
        // of bookmark name and then the name itself follows.
        //
        // Example: FOLD01BOOK000a05mark1001005mark2x001ab
        //
        // Open first level fold, bookmark "mark1" at column 10 and bookmark "mark2" at column 16.
        // At the end is length of string from BOOK up to end of mark2 - 26 (0x1a).

        for (l = RCount - 1; l >= 0; l--) {
            if (LL[l]->Count >= len_start + len_end + 6) {
                for (int where = 1; where < 3; where++) {
                    // where == 1 - start-of-line
                    // where == 2 - end-of-line
                    open = -1;
                    level = -1;
                    if (BFI(this, BFI_SaveFolds) != where && BFI(this, BFI_SaveBookmarks) != where) continue;
                    if (where == 1) {
                        pos = 0;
                    } else {
                        pos = LL[l]->Count - len_end;
                        // Check if line ends with end comment (if defined)
                        if (len_end != 0 && memcmp(LL[l]->Chars + pos, BFS(this, BFS_CommentEnd), len_end) != 0) continue;
                        // Locate position of comment start - skip bookmarks and fold
                        if (BFI(this, BFI_SaveBookmarks) == 2 && pos - 10 >= 0 && LL[l]->Chars[pos-1] == 'b') {
                            char numbuf[5];
                            int i;

                            memcpy(numbuf, LL[l]->Chars + pos - 5, 4); numbuf[4] = 0;
                            if (1 != sscanf(numbuf, "%x", &i)) continue;
                            pos -= i + 6;
                            if (pos < 0) continue;
                        }
                        if (BFI(this, BFI_SaveFolds) == 2 && pos - 6 >= 0 &&
                            (memcmp(LL[l]->Chars + pos - 6, "FOLD", 4) == 0 ||
                             memcmp(LL[l]->Chars + pos - 6, "fold", 4) == 0)) pos -= 6;
                        pos -= len_start;
                    }
                    // Check comment start
                    if (pos < 0 || (len_start != 0 && memcmp(LL[l]->Chars + pos, BFS(this, BFS_CommentStart), len_start) != 0)) continue;
                    // Check comment length - leave empty in file
                    if (LL[l]->Count - pos == len_start + len_end) continue;
                    startpos = pos;
                    pos += len_start;
                    // We have starting position after comment start
                    // Now we will read fold and/or bookmark info and check
                    // for end of comment (if where == 2, it must end at EOLN)

                    // This code is not very good, since on error we stop
                    // parsing comments (and leave it in file), but everything
                    // already done is not undone (e.g. bookmarks, folds)

                    // Folds come always first
                    if (BFI(this, BFI_SaveFolds) == where && (pos + len_end + 6 <= LL[l]->Count)) {
                        if (memcmp(LL[l]->Chars + pos, "FOLD", 4) == 0) {
                            open = 1;
                        } else if (memcmp(LL[l]->Chars + pos, "fold", 4) == 0) {
                            open = 0;
                        } else
                            open = -1;
                    }
                    if (open != -1) {
                        foldnum[0] = LL[l]->Chars[pos + 4];
                        foldnum[1] = LL[l]->Chars[pos + 4 + 1];
                        if (1 != sscanf(foldnum, "%2d", &level))
                            level = -1;

                        if (!isdigit(LL[l]->Chars[pos + 4]) ||
                            !isdigit(LL[l]->Chars[pos + 5]))
                            level = -1;

                        if (level == -1 || open >= 100) continue;
                        pos += 6;
                    }

                    // Now get bookmarks
                    if (BFI(this, BFI_SaveBookmarks) == where && (pos + len_end + 10 <= LL[l]->Count) && memcmp(LL[l]->Chars + pos, "BOOK", 4) == 0) {
                        int error = 0;
                        int i, col, startBook;
                        char numbuf[5], buf[256];

                        startBook = pos; pos += 4;
                        while (pos + len_end + 6 + 6 <= LL[l]->Count) {
                            // Read column
                            memcpy(numbuf, LL[l]->Chars + pos, 4); numbuf[4] = 0;
                            pos += 4;
                            if (1 != sscanf(numbuf, "%x", &col)) {
                                error = 1; break;
                            }
                            // Read length
                            memcpy(numbuf, LL[l]->Chars + pos, 2); numbuf[2] = 0;
                            pos += 2;
                            if (1 != sscanf(numbuf, "%x", &i)) {
                                error = 1; break;
                            }
                            if (pos + i + 6 + len_end > LL[l]->Count || i == 0) {
                                error = 1; break;
                            }
                            if (i) {
                                memcpy(buf, LL[l]->Chars + pos, i); buf[i] = 0;
                                pos += i;
                                if (PlaceUserBookmark(buf, EPoint(l, col)) == 0) goto fail;
                            }
                            if (LL[l]->Chars[pos] == 'x') {
                                // Read total length (just test for correctness)
                                memcpy(numbuf, LL[l]->Chars + pos + 1, 4); numbuf[4] = 0;
                                if (1 != sscanf(numbuf, "%x", &i)) {
                                    error = 1; break;
                                }
                                if (i != pos - startBook || LL[l]->Chars[pos + 5] != 'b') error = 1;
                                else pos += 6;
                                break;
                            }
                        }
                        // Stop parsing this comment if error occured
                        if (error) continue;
                    }

                    // And last: check, if comment is properly terminated
                    if (pos + len_end > LL[l]->Count) continue;
                    if (len_end != 0 && memcmp(LL[l]->Chars + pos, BFS(this, BFS_CommentEnd), len_end) != 0) continue;
                    // Not at EOLN, but should be (comment at EOLN)
                    if (where == 2 && LL[l]->Count != pos + len_end) continue;
                    pos += len_end;

                    // Create fold if whole comment was successfully parsed
                    if (open != -1) {
                        int f;

                        if (!FoldCreate(l))
                            goto fail;
                        f = FindFold(l);
                        assert(f != -1);
                        FF[f].level = (char)(level & 0xFF);
                        if (open == 0)
                            if (!FoldClose(l))
                                goto fail;
                    }
                    // Now remove parsed comment from line
                    memmove(LL[l]->Chars + startpos,
                            LL[l]->Chars + pos,
                            LL[l]->Count - pos);
                    LL[l]->Count -= pos - startpos;
                }
            }
        }
    }
    if (SetPosR(0, 0) == 0) return 0;
    BFI(this, BFI_Undo) = SaveUndo;
    BFI(this, BFI_ReadOnly) = SaveReadOnly;
    if (stat(FileName, &FileStatus) == -1) {
        memset(&FileStatus, 0, sizeof(FileStatus));
        FileOk = 0;
        goto fail;
    } else {
        if (!(FileStatus.st_mode & (S_IWRITE | S_IWGRP | S_IWOTH)))
            BFI(this, BFI_ReadOnly) = 1;
        else
            BFI(this, BFI_ReadOnly) = 0;
    }
    close(fd);

    FileOk = 1;
    Modified = 0;
    Loading = 0;
    Loaded = 1;
    Draw(0, -1);
    Msg(S_INFO, "Loaded %s.", AFileName);
    return 1;
fail:
    free(m);
    BFI(this, BFI_Undo) = SaveUndo;
    BFI(this, BFI_ReadOnly) = SaveReadOnly;
    close(fd);
    Loading = 0;
    Draw(0, -1);
    View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Error loading %s.", AFileName);
    return 0;
}

int EBuffer::SaveTo(const char *AFileName) {
    char ABackupName[MAXPATH];
    struct stat StatBuf;

    FILE *fp;
    int l;
    PELine L;
    unsigned long ByteCount = 0, OldCount = 0;

    int f;
    char fold[64];
    unsigned int foldlen = 0;

    int bindex;
    unsigned int blen = 0;
    char book[1024] = "BOOK";

    unsigned int len_start = 0, len_end = 0;

    if (FileOk && (stat(FileName, &StatBuf) == 0)) {
        if (FileStatus.st_size != StatBuf.st_size ||
            FileStatus.st_mtime != StatBuf.st_mtime)
        {
            switch (View->MView->Win->Choice(GPC_ERROR,
                                             "File Changed on Disk",
                                             2, "&Save", "&Cancel",
                                             "%s", FileName)) {
            case 0:
                break;
            case 1:
            case -1:
            default:
                return 0;
            }
        }
    }

    if (RCount <= 0) return 0;

#ifdef CONFIG_BACKUP
    // make only backups when user have requested a one
    if (BFI(this, BFI_MakeBackups) != 0) {
        Msg(S_INFO, "Backing up %s...", AFileName);
        if (MakeBackup(AFileName, (char *)ABackupName) == 0) {
            View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Could not create backup file.");
            return 0;
        }
    }
#endif

    Msg(S_INFO, "Writing %s...", AFileName);

    fp = 0;
#ifdef OS2 // to preserve EA's ?
    fp = fopen(AFileName, "r+b");
    if (fp != 0)
#if defined(__IBMCPP__) || defined(__WATCOMC__)
        if ( chsize  (fileno(fp), 0) != 0)
#else
        if (ftruncate(fileno(fp), 0) != 0)
#endif // __IBMCPP__ || __WATCOMC__
            goto erroropen;
#endif // OS2
    if (fp == 0)
        fp = fopen(AFileName, "wb");
    if (fp == 0) goto erroropen;

    setvbuf(fp, FileBuffer, _IOFBF, sizeof(FileBuffer));

    // Some initializations (to speed-up saving)
    if (BFS(this, BFS_CommentStart)) len_start = strlen(BFS(this, BFS_CommentStart));
    if (BFS(this, BFS_CommentEnd)) len_end = strlen(BFS(this, BFS_CommentEnd));

    for (l = 0; l < RCount; l++) {
        L = RLine(l);
        f = FindFold(l);

        foldlen = 0;
        // format fold
        if ((f != -1) && (BFI(this, BFI_SaveFolds) != 0)) {
            foldlen = sprintf(fold,
                              FF[f].open ? "FOLD%02d" : "fold%02d",
                              FF[f].level);
        }

        bindex = 0; blen = 0;
#ifdef CONFIG_BOOKMARKS
	if (BFI(this, BFI_SaveBookmarks) == 1 || BFI(this, BFI_SaveBookmarks) == 2) {
            const EBookmark* eb;
            blen = 4;     // Just after "BOOK"
	    while ((bindex = GetUserBookmarkForLine(bindex, l, eb)) != -1) {
                size_t l = strlen(eb->GetName());
                // Skip too long bookmarks
		if (l > 256 || blen + l + 6 + 6 > sizeof(book))
		    continue;
                blen += sprintf(book + blen, "%04x%02x%s", eb->GetPoint().Col, (int)l, eb->GetName());
            }
            if (blen != 4)
                blen += sprintf(book + blen, "x%04xb", blen);
	    else
		blen = 0;      // Signal, that no bookmarks were saved
        }
#endif

        // what - write at 1 = beginning / 2 = end of line
        for (int what = 1; what < 3; what++) {
            if ((BFI(this, BFI_SaveFolds) == what && foldlen) ||
                (BFI(this, BFI_SaveBookmarks) == what && blen)
               ) {

                if (len_start) {
                    if (fwrite(BFS(this, BFS_CommentStart), 1, len_start, fp) != len_start) goto fail;
                    ByteCount += len_start;
                }
                if (BFI(this, BFI_SaveFolds) == what && foldlen) {
                    if (fwrite(fold, 1, foldlen, fp) != foldlen) goto fail;
                    ByteCount += foldlen;
                }
#ifdef CONFIG_BOOKMARKS
                if (BFI(this, BFI_SaveBookmarks) == what && blen) {
                    if (fwrite(book, 1, blen, fp) != blen) goto fail;
                    ByteCount += blen;
                }
#endif
                if (len_end) {
                    if (fwrite(BFS(this, BFS_CommentEnd), 1, len_end, fp) != len_end) goto fail;
                    ByteCount += len_end;
                }
            }
            if (what == 1) {
                // write data
                if ((int)(fwrite(L->Chars, 1, L->Count, fp)) != L->Count)
                    goto fail;
                ByteCount += L->Count;
            }
        }
        // write eol
        if ((l < RCount - 1) || BFI(this, BFI_ForceNewLine)) {
            if (BFI(this, BFI_AddCR) == 1) {
                if (fputc('\r', fp) < 0) goto fail;
                ByteCount++;
            }
            if (BFI(this, BFI_AddLF) == 1) {
                if (fputc('\n', fp) < 0) goto fail;
                ByteCount++;
            }
        }
        if (ByteCount > OldCount + 65536) {
            Msg(S_INFO, "Saving: %d lines, %d bytes.", l, ByteCount);
            OldCount = ByteCount;
        }
    }
    if (fclose(fp) != 0) goto failclose;
    Modified = 0;
    FileOk = 1;
    if (stat(FileName, &FileStatus) == -1) {
        memset(&FileStatus, 0, sizeof(FileStatus));
        FileOk = 0;
        goto failclose;
    }
    Msg(S_INFO, "Wrote %s.", AFileName);

    // make only backups when user have requested a one
    if (BFI(this, BFI_MakeBackups) != 0) {
        if (BFI(this, BFI_KeepBackups) == 0
#ifdef CONFIG_OBJ_CVS
            // No backups for CVS logs
            || this == CvsLogView
#endif
#ifdef CONFIG_OBJ_SVN
            // No backups for SVN logs
            || this == SvnLogView
#endif
           ) {
            unlink(ABackupName);
        }
    }
    return 1;
fail:
    fclose(fp);
failclose:
    unlink(AFileName);
    if (rename(ABackupName, AFileName) == -1) {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Error renaming backup file to original!");
    } else {
        View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Error writing file, backup restored.");
    }
    return 0;
erroropen:
    View->MView->Win->Choice(GPC_ERROR, "Error", 1, "O&K", "Error writing %s (errno=%d).", AFileName, errno);
    return 0;
}
