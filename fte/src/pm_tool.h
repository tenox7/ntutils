#ifndef PM_TOOL_H
#define PM_TOOL_H

#define tiBITMAP     1
#define tiSEPARATOR  2

#define tfDISABLED   1
#define tfDEPRESSED  0x8000

#define WC_MTOOLBAR  "MToolBar"

struct ToolBarItem {
    ULONG ulType;
    ULONG ulId;
    ULONG ulCommand;
    ULONG ulFlags;
    HBITMAP hBitmap;
};

struct ToolBarData {
    USHORT cb;
    LONG ulCount;
    ToolBarItem *pItems;
    LONG ulDepressed;
};

MRESULT EXPENTRY ToolBarProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
void RegisterToolBarClass(HAB hab);
HWND CreateToolBar(HWND parent,
                   HWND owner,
                   ULONG id,
                   ULONG count,
                   ToolBarItem *items);

#endif // PM_TOOL_H
