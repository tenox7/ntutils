
#define InRange(a,x,b) (((a) <= (x)) && ((x) < (b)))
#define Min(a,b) (((a) < (b))?(a):(b))
#define Max(a,b) (((a) > (b))?(a):(b))

#define TYBORDER 6   // border on top and bottom of icon (all of it)
#define TXBORDER 6
#define TXSEPARATOR 4
#define TYICON 24
#define TXICON 24

MRESULT EXPENTRY ToolBarProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) {
    if (msg == WM_CREATE) {
        WinSetWindowPtr(hwnd, QWL_USER, PVOIDFROMMP(mp1));
    } else {
        ToolBarData *td = (ToolBarData *)WinQueryWindowPtr(hwnd, QWL_USER);
        ToolBarItem *items = td->pItems;
        
        switch (msg) {
        case WM_DESTROY:
            free(td);
            free(items);
            break;

        case WM_PAINT:
            {
                HPS hps;
                RECTL rcl;
                POINTL ptl;
                SWP swp;
                int xpos, ypos, item;

                WinQueryWindowPos(hwnd, &swp);
                
                hps = WinBeginPaint(hwnd, 0, &rcl);

                /* top outside 3D border */
                if (rcl.yBottom < 1) {
                    GpiSetColor(hps, CLR_DARKGRAY);
                    ptl.x = rcl.xLeft;
                    ptl.y = 0;
                    GpiMove(hps, &ptl);
                    ptl.x = Min(rcl.xRight, swp.cx - 2);
                    GpiLine(hps, &ptl);
                }

                /* bottom outside 3D border */
                if (rcl.yTop >= swp.cy - 1 - 1) {
                    GpiSetColor(hps, CLR_WHITE);
                    ptl.x = Max(rcl.xLeft, 1);
                    ptl.y = swp.cy - 1;
                    GpiMove(hps, &ptl);
                    ptl.x = rcl.xRight;
                    GpiLine(hps, &ptl);
                }

                /* 3D corners */
                GpiSetColor(hps, CLR_PALEGRAY);
                ptl.x = 0;
                ptl.y = 0;
                GpiSetPel(hps, &ptl);
                ptl.x = swp.cx - 1;
                ptl.y = swp.cy - 1;
                GpiSetPel(hps, &ptl);

                /* bottom space */
                if (rcl.yBottom < TYBORDER - 1) {
                    for (ptl.y = 1; ptl.y < TYBORDER - 2; ptl.y++) {
                        ptl.x = Max(rcl.xLeft, 1);
                        GpiMove(hps, &ptl);
                        ptl.x = Min(rcl.xRight, swp.cx - 1);
                        GpiLine(hps, &ptl);
                    }
                }
                
                /* top space */
                if (rcl.yTop >= swp.cy - TYBORDER + 2) {
                    for (ptl.y = swp.cy - TYBORDER + 2; ptl.y < swp.cy - 1; ptl.y++) {
                        ptl.x = Max(rcl.xLeft, 1);
                        GpiMove(hps, &ptl);
                        ptl.x = Min(rcl.xRight, swp.cx - 1);
                        GpiLine(hps, &ptl);
                    }
                }

                /* left outside 3D border */
                if (rcl.xLeft < 1) {
                    GpiSetColor(hps, CLR_WHITE);
                    ptl.y = Max(1, rcl.yBottom);
                    ptl.x = 0;
                    GpiMove(hps, &ptl);
                    ptl.y = rcl.yTop;
                    GpiLine(hps, &ptl);
                }

                /* right outside 3D border */
                if (rcl.xRight >= swp.cx - 1) {
                    GpiSetColor(hps, CLR_DARKGRAY);
                    ptl.y = rcl.yBottom;
                    ptl.x = swp.cx - 1;
                    GpiMove(hps, &ptl);
                    ptl.y = Min(swp.cy - 2, rcl.yTop);
                    GpiLine(hps, &ptl);
                }

                /* left border */
                if (rcl.xLeft < TXBORDER - 2) {
                    GpiSetColor(hps, CLR_PALEGRAY);
                    for (ptl.x = 1; ptl.x < TXBORDER - 2; ptl.x++) {
                        ptl.y = Max(1, rcl.yBottom);
                        GpiMove(hps, &ptl);
                        ptl.y = Min(swp.cy - 2, rcl.yTop);
                        GpiLine(hps, &ptl);
                    }
                }

                /* draw toolbar items */
                xpos = TXBORDER;
                ypos = TYBORDER;

                for (item = 0; item < td->ulCount; item++) {
                    if (items[item].ulType == tiBITMAP) {
                        if (rcl.xRight >= xpos - 2 && rcl.xLeft <= xpos + TXICON + 1) {
                            GpiSetColor(hps, CLR_BLACK);
                            ptl.x = xpos - 2;
                            ptl.y = ypos - 2;
                            GpiMove(hps, &ptl);
                            ptl.x = xpos + TXICON + 1;
                            ptl.y = ypos + TYICON + 1;
                            GpiBox(hps, DRO_OUTLINE, &ptl, 0, 0);
                            if (item == td->ulDepressed && (items[item].ulFlags & tfDEPRESSED)) {
                                ptl.x = xpos + 1;
                                ptl.y = ypos - 1;
                                WinDrawBitmap(hps,
                                              items[item].hBitmap,
                                              0,
                                              &ptl,
                                              0, 0,
                                              (items[item].ulFlags & tfDISABLED) ? DBM_INVERT: DBM_NORMAL);
    
                                GpiSetColor(hps, CLR_DARKGRAY);
                                ptl.x = xpos - 1;
                                ptl.y = ypos - 1;
                                GpiMove(hps, &ptl);
                                ptl.y = ypos + TYICON;
                                GpiLine(hps, &ptl);
                                ptl.x = xpos + TXICON;
                                GpiLine(hps, &ptl);
                                ptl.y--;
                                GpiMove(hps, &ptl);
                                ptl.x = xpos;
                                GpiLine(hps, &ptl);
                                ptl.y = ypos - 1;
                                GpiLine(hps, &ptl);
                            } else {
                                ptl.x = xpos;
                                ptl.y = ypos;
                                WinDrawBitmap(hps,
                                              items[item].hBitmap,
                                              0,
                                              &ptl,
                                              0, 0,
                                              (items[item].ulFlags & tfDISABLED) ? DBM_INVERT: DBM_NORMAL);
    
                                GpiSetColor(hps, CLR_PALEGRAY);
                                ptl.x = xpos - 1;
                                ptl.y = ypos - 1;
                                GpiSetPel(hps, &ptl);
                                GpiSetColor(hps, CLR_WHITE);
                                ptl.y++;
                                GpiMove(hps, &ptl);
                                ptl.y = ypos + TYICON;
                                GpiLine(hps, &ptl);
                                ptl.x = xpos + TXICON - 1;
                                GpiLine(hps, &ptl);
                                GpiSetColor(hps, CLR_PALEGRAY);
                                ptl.x++;
                                GpiSetPel(hps, &ptl);
                                ptl.y--;
                                GpiSetColor(hps, CLR_DARKGRAY);
                                GpiMove(hps, &ptl);
                                ptl.y = ypos - 1;
                                GpiLine(hps, &ptl);
                                ptl.x = xpos;
                                GpiLine(hps, &ptl);
                            }
                        }
                        xpos += TXICON + 3;
                    } else if (items[item].ulType == tiSEPARATOR) {
                        if (rcl.xRight >= xpos - 1 && rcl.xLeft <= xpos + TXSEPARATOR + 1) {
                            GpiSetColor(hps, CLR_PALEGRAY);
                            ptl.x = xpos - 1;
                            ptl.y = ypos - 2;
                            GpiMove(hps, &ptl);
                            ptl.x = xpos + TXSEPARATOR + 1;
                            ptl.y = ypos + TYICON + 1;
                            GpiBox(hps, DRO_FILL, &ptl, 0, 0);
                        }
                        xpos += TXSEPARATOR + 3;
                    }
                }
                GpiSetColor(hps, CLR_PALEGRAY);
                ptl.x = xpos - 1;
                ptl.y = ypos - 2;
                GpiMove(hps, &ptl);
                ptl.x = swp.cx - 2;
                ptl.y = swp.cy - TYBORDER + 1;
                GpiBox(hps, DRO_FILL, &ptl, 0, 0);

                WinEndPaint(hps);
            }
            break;

        case WM_ADJUSTWINDOWPOS:
            {
                PSWP pswp = (PSWP)PVOIDFROMMP(mp1);
                pswp->cy = TYBORDER + TYICON + TYBORDER;
            }
            break;
            
        case WM_BUTTON1DOWN:
        case WM_BUTTON1DBLCLK:
            {
                int item;
                POINTL ptl;
                RECTL rcl;
                
                ptl.x = (LONG) SHORT1FROMMP(mp1);
                ptl.y = (LONG) SHORT2FROMMP(mp1);

                rcl.yBottom = TYBORDER - 1;
                rcl.yTop = TYBORDER + TYICON + 1;
                rcl.xLeft = TXBORDER - 1;
                rcl.xRight = TXBORDER + TXICON + 1;

                for (item = 0; item < td->ulCount; item++) {
                    if (rcl.xLeft <= ptl.x && rcl.yBottom <= ptl.y &&
                        rcl.xRight >= ptl.x && rcl.yTop >= ptl.y &&
                        td->pItems[item].ulType == tiBITMAP &&
                        (td->pItems[item].ulFlags & tfDISABLED) == 0)
                    {
                        td->ulDepressed = item;
                        td->pItems[item].ulFlags |= tfDEPRESSED;
                        WinInvalidateRect(hwnd, &rcl, FALSE);
                        WinSetCapture(HWND_DESKTOP, hwnd);
                        break;
                    }
                    if (td->pItems[item].ulType == tiBITMAP) {
                        rcl.xLeft += TXICON + 3;
                        rcl.xRight += TXICON + 3;
                    } else if (td->pItems[item].ulType == tiSEPARATOR) {
                        rcl.xLeft += TXSEPARATOR + 3;
                        rcl.xRight += TXSEPARATOR + 3;
                    }
                }
            }
            break;
            
        case WM_MOUSEMOVE:
            {
                STARTFUNC("ToolBarProc[WM_MOUSEMOVE]");
                int item;
                POINTL ptl;
                RECTL rcl;

                if (td->ulDepressed == -1)
                    break;

                ptl.x = (LONG) SHORT1FROMMP(mp1);
                ptl.y = (LONG) SHORT2FROMMP(mp1);

                rcl.yBottom = TYBORDER - 1;
                rcl.yTop = TYBORDER + TYICON + 1;
                rcl.xLeft = TXBORDER - 1;
                rcl.xRight = TXBORDER + TXICON + 1;

                LOG << "Depressed: " << td-> ulDepressed << ENDLINE;
                for (item = 0; item < td->ulCount; item++) {
                    LOG << "Checking item " << item << ENDLINE;
                    LOG << "  pItem -> " << (void*)(td->pItems + item) << ENDLINE;
                    if (item == td->ulDepressed) {
                        if (rcl.xLeft <= ptl.x && rcl.yBottom <= ptl.y &&
                            rcl.xRight >= ptl.x && rcl.yTop >= ptl.y)
                        {
                            if ((td->pItems[item].ulFlags & tfDEPRESSED) == 0) {
                                td->pItems[item].ulFlags |= tfDEPRESSED;
                                WinInvalidateRect(hwnd, &rcl, FALSE);
                            }
                        } else {
                            if (td->pItems[item].ulFlags & tfDEPRESSED) {
                                td->pItems[item].ulFlags &= ~tfDEPRESSED;
                                WinInvalidateRect(hwnd, &rcl, FALSE);
                            }
                        }
                        break;
                    }
                    if (td->pItems[item].ulType == tiBITMAP) {
                        rcl.xLeft += TXICON + 3;
                        rcl.xRight += TXICON + 3;
                    } else if (td->pItems[item].ulType == tiSEPARATOR) {
                        rcl.xLeft += TXSEPARATOR + 3;
                        rcl.xRight += TXSEPARATOR + 3;
                    }
                }
            }
            break;
            
        case WM_BUTTON1UP:
            {
                int item;
                POINTL ptl;
                RECTL rcl;

                if (td->ulDepressed == -1)
                    break;

                ptl.x = (LONG) SHORT1FROMMP(mp1);
                ptl.y = (LONG) SHORT2FROMMP(mp1);

                rcl.yBottom = TYBORDER - 1;
                rcl.yTop = TYBORDER + TYICON + 1;
                rcl.xLeft = TXBORDER - 1;
                rcl.xRight = TXBORDER + TXICON + 1;

                for (item = 0; item < td->ulCount; item++) {
                    if (item == td->ulDepressed) {
                        WinSetCapture(HWND_DESKTOP, (HWND)0);
                        if (rcl.xLeft <= ptl.x && rcl.yBottom <= ptl.y &&
                            rcl.xRight >= ptl.x && rcl.yTop >= ptl.y &&
                            td->pItems[item].ulFlags & tfDEPRESSED)
                        {
                            td->pItems[item].ulFlags &= ~tfDEPRESSED;
                            WinInvalidateRect(hwnd, &rcl, FALSE);

                            // message
                            WinPostMsg(WinQueryWindow(hwnd, QW_OWNER), WM_COMMAND, MPFROM2SHORT(td->pItems[item].ulCommand, 0), MPFROM2SHORT(CMDSRC_OTHER, TRUE));

                            break;
                         }
                    }
                    if (td->pItems[item].ulType == tiBITMAP) {
                        rcl.xLeft += TXICON + 3;
                        rcl.xRight += TXICON + 3;
                    } else if (td->pItems[item].ulType == tiSEPARATOR) {
                        rcl.xLeft += TXSEPARATOR + 3;
                        rcl.xRight += TXSEPARATOR + 3;
                    }
                }
                td->ulDepressed = -1;
            }
            break;
        }
    }
    return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

void RegisterToolBarClass(HAB hab) {
    assert(WinRegisterClass(hab,
                            WC_MTOOLBAR,
                            (PFNWP)ToolBarProc,
                            CS_SIZEREDRAW,
                            sizeof(void *)) == TRUE);
}

HWND CreateToolBar(HWND parent,
                   HWND owner,
                   ULONG id,
                   ULONG count,
                   ToolBarItem *items)
{
    STARTFUNC("CreateToolBar");
    ToolBarData *td;
    HWND hwnd;

    td = (ToolBarData *)malloc(sizeof(ToolBarData));
    if (td == 0)
        return 0;
    td->pItems = (ToolBarItem *)malloc(sizeof(ToolBarItem) * count);
    if (td->pItems == 0)
    {
        free(td);
        return 0;
    }

    td->cb = sizeof(ToolBarData);
    td->ulCount = count;
    td->ulDepressed = (LONG)-1;
    memcpy((void *)td->pItems, (void *)items, sizeof(ToolBarItem) * count);

    hwnd = WinCreateWindow(parent,
                           WC_MTOOLBAR,
                           "ToolBar",
                           WS_VISIBLE,
                           0, 0, 0, 0,
                           owner,
                           HWND_TOP,
                           id,
                           td,
                           0);

    //free(td); <-- Don't do this here as now the window owns the memory!
    return hwnd;
}
