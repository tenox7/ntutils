/*    i_oview.cpp
 *
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "i_view.h"

ExView::ExView() :
    Win(0),
    Next(0)
{
}

ExView::~ExView()
{
}

void ExView::Activate(int /*gotfocus*/) {
}

int ExView::IsActive() {
    return (Win) ? Win->IsActive() : 0;
}

int ExView::GetContext()
{
    return CONTEXT_NONE;
}

ExView *ExView::GetViewContext()
{
    return this;
}

ExView *ExView::GetStatusContext()
{
    return this;
}

EEventMap *ExView::GetEventMap()
{
    return 0;
}

int ExView::ExecCommand(ExCommands /*Command*/, ExState &/*State*/) { return 0; }

int ExView::BeginMacro() {
    return 1;
}

void ExView::HandleEvent(TEvent &Event) {
    if (Event.What == evKeyDown && kbCode(Event.Key.Code) == kbF12)
        Win->Parent->SelectNext(0);
}

void ExView::EndExec(int NewResult) {
    if (Win->Result == -2) { // hack
        Win->EndExec(NewResult);
    } else {
        if (Next)
            delete Win->PopView(); // self
    }
}

void ExView::UpdateView() {
}

void ExView::UpdateStatus() {
}

void ExView::RepaintView() {
}

void ExView::RepaintStatus() {
}

void ExView::Repaint()
{
    RepaintStatus();
    RepaintView();
}

void ExView::Update()
{
    UpdateStatus();
    UpdateView();
}

void ExView::Resize(int /*width*/, int /*height*/) {
    Repaint();
}

int ExView::ConPutBox(int X, int Y, int W, int H, PCell Cell) {
    return (Win) ? Win->ConPutBox(X, Y, W, H, Cell) : -1;
}

int ExView::ConScroll(int Way, int X, int Y, int W, int H, TAttr Fill, int Count) {
    return (Win) ? Win->ConScroll(Way, X, Y, W, H, Fill, Count) : -1;
}

int ExView::ConQuerySize(int *X, int *Y) {
    return (Win) ? Win->ConQuerySize(X, Y) : -1;
}

int ExView::ConSetCursorPos(int X, int Y) {
    return (Win) ? Win->ConSetCursorPos(X, Y) : -1;
}

int ExView::ConShowCursor() {
    return (Win) ? Win->ConShowCursor() : -1;
}

int ExView::ConHideCursor() {
    return (Win) ? Win->ConHideCursor() : -1;
}

int ExView::ConSetCursorSize(int Start, int End) {
    return (Win) ? Win->ConSetCursorSize(Start, End) : -1;
}

int ExView::IsModelView()
{
    return 0;
}

ExView* ExViewNext::GetViewContext()
{
    return Next;
}

void ExViewNext::RepaintView()
{
    if (Next)
        Next->RepaintView();
}

void ExViewNext::UpdateView()
{
    if (Next)
        Next->UpdateView();
}

void ExViewNext::UpdateStatus()
{
    RepaintStatus();
}
