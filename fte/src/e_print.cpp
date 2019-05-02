/*    e_print.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "c_config.h"
#include "o_buflist.h"
#include "s_util.h"

#include <stdio.h>

int EBuffer::BlockPrint() {
    const char cr = '\r';
    const char lf = '\n';
    EPoint B, E;
    int L;
    int A, Z;
    PELine LL;
    FILE *fp;
    int bc = 0, lc = 0;
    int error = 0;
    
    AutoExtend = 0;
    if (CheckBlock() == 0) return 0;
    if (RCount == 0) return 0;
    B = BB;
    E = BE;
    Msg(S_INFO, "Printing to %s...", PrintDevice);
#if !defined(__IBMCPP__) && !defined(__WATCOMC__)
    if (PrintDevice[0] == '|')
        fp = popen(PrintDevice + 1, "w");
    else
#endif
        fp = fopen(PrintDevice, "w");
    if (fp == NULL) {
        Msg(S_INFO, "Failed to write to %s", PrintDevice);
        return 0;
    }
    for (L = B.Row; L <= E.Row; L++) {
        A = -1;
        Z = -1;
        LL = RLine(L);
        switch (BlockMode) {
          case bmLine:
            if (L < E.Row) {
                A = 0;
                Z = LL->Count;
            }
            break;
          case bmColumn:
            if (L < E.Row) {
                A = CharOffset(LL, B.Col);
                Z = CharOffset(LL, E.Col);
            }
            break;
          case bmStream:
            if (B.Row == E.Row) {
                A = CharOffset(LL, B.Col);
                Z = CharOffset(LL, E.Col);
            } else if (L == B.Row) {
                A = CharOffset(LL, B.Col);
                Z = LL->Count;
            } else if (L < E.Row) {
                A = 0;
                Z = LL->Count;
            } else if (L == E.Row) {
                A = 0;
                Z = CharOffset(LL, E.Col);
            }
            break;
        }
        if (A != -1 && Z != -1) {
            if (A < LL->Count) {
                if (Z > LL->Count)
                    Z = LL->Count;
                if (Z > A) {
                    if ((int)(fwrite(LL->Chars + A, 1, Z - A, fp)) != Z - A) {
                        error++;
                        break;
                    } else
                        bc += Z - A;
                }
            }
            if (BFI(this, BFI_AddCR) == 1) {
                if (fwrite(&cr, 1, 1, fp) != 1) {
                    error++;
                    break;
                } else
                    bc++;
            }
            if (BFI(this, BFI_AddLF) == 1) {
                if (fwrite(&lf, 1, 1, fp) != 1) {
                    error++;
                    break;
                } else {
                    bc++;
                    lc++;
                }
            }
            if ((lc % 200) == 0)
                Msg(S_INFO, "Printing, %d lines, %d bytes.", lc, bc);

        }
    }
    if (!error) {
        fwrite("\f\n", 2, 1, fp);
#if !defined(__IBMCPP__) && !defined(__WATCOMC__)
        if (PrintDevice[0] == '|')
            pclose(fp);
        else
#endif
            fclose(fp);
        Msg(S_INFO, "Printing %d lines, %d bytes.", lc, bc);
        return 1;
    }
#if !defined(__IBMCPP__) && !defined(__WATCOMC__)
    if (PrintDevice[0] == '|')
        pclose(fp);
    else
#endif
        fclose(fp);
    Msg(S_INFO, "Failed to write to %s", PrintDevice);
    return 0;
}

int EBuffer::FilePrint() {
    const char cr = 13;
    const char lf = 10;
    int l;
    FILE *fp;
    unsigned long ByteCount = 0;
    int BChars;
  
    Msg(S_INFO, "Printing %s to %s...", FileName, PrintDevice);
#if !defined(__IBMCPP__) && !defined(__WATCOMC__)
    if (PrintDevice[0] == '|')
        fp = popen(PrintDevice + 1, "w");
    else
#endif
        fp = fopen(PrintDevice, "w");
    if (fp == NULL) {
        Msg(S_ERROR, "Error printing %s to %s.", FileName, PrintDevice);
        return 0;
    }
    BChars = 0;
    for (l = 0; l < RCount; l++) {
        if ((int) sizeof(FileBuffer) - (BChars + 2) < RLine(l)->Count) {
            if (BChars) {
                ByteCount += BChars;
                Msg(S_INFO, "Printing: %d lines, %d bytes.", l, ByteCount);
                if ((int)(fwrite(FileBuffer, 1, BChars, fp)) != BChars) goto fail;
                BChars = 0;
            }
        }
        if (RLine(l)->Count > int(sizeof(FileBuffer)) - 2) {
            assert(BChars == 0);
            ByteCount += RLine(l)->Count;
            Msg(S_INFO, "Printing: %d lines, %d bytes.", l, ByteCount);
            if (int(fwrite(RLine(l)->Chars, 1, RLine(l)->Count, fp)) != RLine(l)->Count) goto fail;
        } else {
            memcpy(FileBuffer + BChars, RLine(l)->Chars, RLine(l)->Count);
            BChars += RLine(l)->Count;
        }
        if ((l < RCount - 1) || BFI(this, BFI_ForceNewLine)) {
            assert(int(sizeof(FileBuffer)) >= BChars + 2);
            if (BFI(this, BFI_AddCR) == 1) FileBuffer[BChars++] = cr;
            if (BFI(this, BFI_AddLF) == 1) FileBuffer[BChars++] = lf;
        }
    }
    if (BChars) {
        ByteCount += BChars;
        Msg(S_INFO, "Printing: %d lines, %d bytes.", l, ByteCount);
        if ((int)(fwrite(FileBuffer, 1, BChars, fp)) != BChars) goto fail;
    }
    BChars = 0;
#if !defined(__IBMCPP__) && !defined(__WATCOMC__)
    if (PrintDevice[0] == '|')
        pclose(fp);
    else
#endif
        fclose(fp);
    Msg(S_INFO, "Printed %s.", FileName);
    return 1;
fail:
    if (fp != NULL) {
#if !defined(__IBMCPP__) && !defined(__WATCOMC__)
        if (PrintDevice[0] == '|')
            pclose(fp);
        else
#endif
            fclose(fp);
    }
    Msg(S_ERROR, "Error printing %s to %s.", FileName, PrintDevice);
    return 0;
}
