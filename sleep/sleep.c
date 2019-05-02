// Unix sleep(1) equivalent for Win32
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>

int wmain(DWORD argc, WCHAR **argv) {
    DWORD seconds=0;
    WCHAR *stopchar;

    if(argc!=2) {
        wprintf(L"Sleep for Win32, v1.2\n\nUsage: sleep <seconds>\n\n");
        return 1;
    }

    seconds = wcstoul(argv[1], &stopchar, 10);

    wprintf(L"Sleeping for %u seconds...", seconds);

    Sleep(seconds * 1000);

    wprintf(L"\n");
    
    return 0;
}