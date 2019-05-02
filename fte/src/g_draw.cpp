/*    g_draw.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "console.h"

#include <stdlib.h>
#ifdef  NTCONSOLE
#   define  WIN32_LEAN_AND_MEAN 1
#   include <windows.h>
#endif

size_t CStrLen(const char *p) {
    size_t len = 0;
    while (*p) {
	if (*p++ == '&') {
	    if (*p != '&')
		continue;
	    p++;
	}
	len++; // && is one accounted as '&'
    }
    return len;
}

#ifndef NTCONSOLE

void MoveCh(PCell B, char CCh, TAttr Attr, size_t Count) {
    assert((int)Count >= 0);
    for (;Count > 0; B++, --Count)
        B->Set(CCh, Attr);
}

void MoveChar(PCell B, int Pos, int Width, const char CCh, TAttr Attr, size_t Count) {
    assert((int)Count >= 0);
    if (Pos < 0) {
	if ((int)Count < -Pos)
            return;
        Count += Pos;
        Pos = 0;
    }
    if (Pos >= Width) return;
    if ((int)Count > Width - Pos) Count = Width - Pos;
    for (B += Pos; Count > 0; B++, --Count)
        B->Set(CCh, Attr);
}

void MoveMem(PCell B, int Pos, int Width, const char* Ch, TAttr Attr, size_t Count) {
    assert((int)Count >= 0);
    if (Pos < 0) {
	if ((int)Count < -Pos)
            return;
        Count += Pos;
        Ch -= Pos;
        Pos = 0;
    }
    if (Pos >= Width) return;
    if ((int)Count > Width - Pos) Count = Width - Pos;
    for (B += Pos; Count > 0; B++, --Count)
        B->Set(*Ch++, Attr);
}

void MoveStr(PCell B, int Pos, int Width, const char* Ch, TAttr Attr, size_t MaxCount) {
    assert((int)MaxCount >= 0);
    if (Pos < 0) {
	if ((int)MaxCount < -Pos)
            return;
        MaxCount += Pos;
        Ch -= Pos;
        Pos = 0;
    }
    if (Pos >= Width) return;
    if ((int)MaxCount > Width - Pos) MaxCount = Width - Pos;
    for (B += Pos; MaxCount > 0 && *Ch; B++, --MaxCount)
        B->Set(*Ch++, Attr);
}

void MoveCStr(PCell B, int Pos, int Width, const char* Ch, TAttr A0, TAttr A1, size_t MaxCount) {
    assert((int)MaxCount >= 0);
    TAttr attr = A0;
    if (Pos < 0) {
	if ((int)MaxCount < -Pos)
            return;
        MaxCount += Pos;
        Ch -= Pos;
        Pos = 0;
    }
    if (Pos >= Width) return;
    if ((int)MaxCount > Width - Pos) MaxCount = Width - Pos;
    for (B += Pos; MaxCount > 0 && *Ch;) {
        if (*Ch == '&' && attr == A0) {
            Ch++;
            attr = A1;
            continue;
        } 
        B++->Set(*Ch++, attr);
        attr = A0;
        --MaxCount;
    }
}

void MoveAttr(PCell B, int Pos, int Width, TAttr Attr, size_t Count) {
    assert((int)Count >= 0);
    if (Pos < 0) {
	if ((int)Count < -Pos)
            return;
        Count += Pos;
        Pos = 0;
    }
    if (Pos >= Width) return;
    if ((int)Count > Width - Pos) Count = Width - Pos;
    for (B += Pos; Count > 0; B++, --Count)
        B->SetAttr(Attr);
}

void MoveBgAttr(PCell B, int Pos, int Width, TAttr Attr, size_t Count) {
    assert((int)Count >= 0);
    if (Pos < 0) {
	if ((int)Count < -Pos)
            return;
        Count += Pos;
        Pos = 0;
    }
    if (Pos >= Width) return;
    if ((int)Count > Width - Pos) Count = Width - Pos;
    for (B += Pos; Count > 0; B++, Count--)
        B->SetAttr(TAttr((B->GetAttr() & 0x0F) | Attr));
}

#else

void MoveCh(PCell B, char Ch, TAttr Attr, size_t Count) {
    PCHAR_INFO p = (PCHAR_INFO) B;
    while (Count > 0) {
        p->Char.AsciiChar = Ch;
        p->Attributes = Attr;
        p++;
        Count--;
    }
}

void MoveChar(PCell B, int Pos, int Width, const char Ch, TAttr Attr, size_t Count) {
    PCHAR_INFO p = (PCHAR_INFO) B;
    if (Pos < 0) {
        Count += Pos;
        Pos = 0;
    }
    if (Pos >= Width) return;
    if (Pos + Count > Width) Count = Width - Pos;
    for (p += Pos; Count > 0; Count--) {
        p->Char.AsciiChar = Ch;
        p->Attributes = Attr;
        p++;
    }
}

void MoveMem(PCell B, int Pos, int Width, const char* Ch, TAttr Attr, size_t Count) {
    PCHAR_INFO p = (PCHAR_INFO) B;
    
    if (Pos < 0) {
        Count += Pos;
        Ch -= Pos;
        Pos = 0;
    }
    if (Pos >= Width) return;
    if (Pos + Count > Width) Count = Width - Pos;
    for (p += Pos; Count > 0; p++, --Count) {
        p->Char.AsciiChar = *Ch++;
        p->Attributes = Attr;
    }
}

void MoveStr(PCell B, int Pos, int Width, const char* Ch, TAttr Attr, size_t MaxCount) {
    PCHAR_INFO p = (PCHAR_INFO) B;
    
    if (Pos < 0) {
        MaxCount += Pos;
        Ch -= Pos;
        Pos = 0;
    }
    if (Pos >= Width) return;
    if (Pos + MaxCount > Width) MaxCount = Width - Pos;
    for (p += Pos; MaxCount > 0 && (*Ch != 0); MaxCount--) {
        p->Char.AsciiChar = *Ch++;
        p->Attributes = Attr;
        p++;
    }
}

void MoveCStr(PCell B, int Pos, int Width, const char* Ch, TAttr A0, TAttr A1, size_t MaxCount) {
    PCHAR_INFO p = (PCHAR_INFO) B;
    char was;
    //TAttr A;
    
    if (Pos < 0) {
        MaxCount += Pos;
        Ch -= Pos;
        Pos = 0;
    }
    if (Pos >= Width) return;
    if (Pos + MaxCount > Width) MaxCount = Width - Pos;
    was = 0;
    for (p += Pos; MaxCount > 0 && (*Ch != 0); MaxCount--) {
        if (*Ch == '&' && !was) {
            Ch++;
            MaxCount++;
            was = 1;
            continue;
        } 
        p->Char.AsciiChar = (unsigned char) (*Ch++);
        if (was) {
            p->Attributes = A1;
            was = 0;
        } else
            p->Attributes = A0;
        p++;
    }
}

void MoveAttr(PCell B, int Pos, int Width, TAttr Attr, size_t Count) {
    PCHAR_INFO p = (PCHAR_INFO) B;
    
    if (Pos < 0) {
        Count += Pos;
        Pos = 0;
    }
    if (Pos >= Width) return;
    if (Pos + Count > Width) Count = Width - Pos;
    for (p += Pos; Count > 0; p++, --Count)
        p->Attributes = Attr;
}

void MoveBgAttr(PCell B, int Pos, int Width, TAttr Attr, size_t Count) {
    PCHAR_INFO p = (PCHAR_INFO) B;
    
    if (Pos < 0) {
        Count += Pos;
        Pos = 0;
    }
    if (Pos >= Width) return;
    if (Pos + Count > Width) Count = Width - Pos;
    for (p += Pos; Count > 0; p++, --Count)
        p->Attributes =
            ((unsigned char)(p->Attributes & 0xf)) |
            ((unsigned char) Attr);
}

#endif
