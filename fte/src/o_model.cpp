/*    o_model.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "o_model.h"

#include "c_config.h"
#include "i_modelview.h"
#include "i_view.h"
#include "s_util.h"

#include <stdio.h> // vsnprintf

EModel* ActiveModel = 0;

EModel *FindModelID(EModel *Model, int ID) {
    for (EModel *M = Model; M; M = M->Next) {
        if (M->ModelNo == ID)
            return M;
        if (M->Next == Model)
            break;
    }
    return 0;
}

static int GetNewModelID(EModel *B) {
    static int lastid = -1;

    if (ReassignModelIds) lastid = 0;   // 0 is used by buffer list
    while (FindModelID(B, ++lastid) != 0)
	;

    return lastid;
}

EModel::EModel(int createFlags, EModel **ARoot) :
    Root(ARoot),
    View(0),
    ModelNo(-1)
{
    if (Root) {
        if (*Root) {
            if (createFlags & cfAppend) {
                Prev = *Root;
                Next = (*Root)->Next;
            } else {
                Next = *Root;
                Prev = (*Root)->Prev;
            }
            Prev->Next = this;
            Next->Prev = this;
        } else
            Prev = Next = this;

        if (!(createFlags & cfNoActivate))
            *Root = this;
    } else
        Prev = Next = this;

    ModelNo = GetNewModelID(this);
}

EModel::~EModel() {
    //printf("Delete model  %p\n", this);
    for (EModel *D = this; D; D = D->Next) {
        D->NotifyDelete(this);
        if (D->Next == this)
            break;
    }

    if (Next != this) {
        Prev->Next = Next;
        Next->Prev = Prev;
        if (*Root == this)
            *Root = Next;
    } else
        *Root = 0;
}

void EModel::AddView(EView *V) {
    //fprintf(stderr, "****Add view %p n:(%p)  -> %p\n", V, V ? V->NextView : NULL, this);
    RemoveView(V);
    if (V)
        V->NextView = View;
    View = V;
    //fprintf(stderr, "Add view %p  -> %p\n", View, this);
}

void EModel::RemoveView(EView *V) {
    EView **X = &View;

    //fprintf(stderr, "Remove view %p n:(%p) <- %p, v:%p\n", V, V ? V->NextView : NULL, this, View);
    if (!V) return;
    while (*X) {
        if ((*X) == V) {
	    //(*X)->NextView = 0;
	    *X = V->NextView;
            return;
        }
        X = (&(*X)->NextView);
    }
}

void EModel::SelectView(EView *V) {
    RemoveView(V);
    AddView(V);
}

EViewPort *EModel::CreateViewPort(EView * /*V*/) {
    return 0;
}

int EModel::ExecCommand(ExCommands /*Command*/, ExState &/*State*/) {
    return 0;
}

void EModel::HandleEvent(TEvent &/*Event*/) {
}

void EModel::Msg(int level, const char *s, ...) {
    char msgbuftmp[MSGBUFTMP_SIZE];
    va_list ap;

    if (View == 0)
        return;

    va_start(ap, s);
    vsnprintf(msgbuftmp, sizeof(msgbuftmp), s, ap);
    va_end(ap);

    if (level != S_BUSY)
        View->SetMsg(msgbuftmp);
}

int EModel::CanQuit() {
    return 1;
}

int EModel::ConfQuit(GxView * /*V*/, int /*multiFile*/) {
    return 1;
}

int EModel::GetContext() { return CONTEXT_NONE; }
EEventMap *EModel::GetEventMap() { return 0; }
int EModel::BeginMacro() { return 1; }
void EModel::GetName(char *AName, size_t /*MaxLen*/) { *AName = 0; }
void EModel::GetPath(char *APath, size_t /*MaxLen*/) { *APath = 0; }
void EModel::GetInfo(char *AInfo, size_t /*MaxLen*/) { *AInfo = 0; }
void EModel::GetTitle(char *ATitle, size_t /*MaxLen*/, char *ASTitle, size_t /*SMaxLen*/) { *ATitle = 0; *ASTitle = 0; }
void EModel::NotifyPipe(int /*PipeId*/) { }

void EModel::NotifyDelete(EModel * /*Deleted*/) {
}
void EModel::DeleteRelated() {
}

void EModel::UpdateTitle() {
    char Title[256];
    char STitle[256];

    GetTitle(Title, sizeof(Title), STitle, sizeof(STitle));

    for (EView *V = View; V; V = V->NextView)
	V->MView->Win->UpdateTitle(Title, STitle);
}

int EModel::GetStrVar(int var, char *str, size_t buflen) {
    switch (var) {
    case mvCurDirectory:
        return GetDefaultDirectory(this, str, buflen);
    }
    return 0;
}

int EModel::GetIntVar(int /*var*/, int * /*value*/) {
    return 0;
}


EViewPort::EViewPort(EView *V) :
    View(V),
    ReCenter(0)
{
}
EViewPort::~EViewPort() {}
void EViewPort::HandleEvent(TEvent &/*Event*/) { }
void EViewPort::UpdateView() { }
void EViewPort::RepaintView() { }
void EViewPort::UpdateStatus() { }
void EViewPort::RepaintStatus() { }
void EViewPort::GetPos() { }
void EViewPort::StorePos() { }
void EViewPort::Resize(int /*Width*/, int /*Height*/) {}
