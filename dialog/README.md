# Dialog

This is a port of Thomas E. Dickey [Dialog](https://invisible-island.net/dialog/) to Windows NT Console.

It works suprisingly well with Windows CMD/BAT scripts. 

This example lets you pick WIM file for DISM in WinPE using Dialog Menu:

```
setlocal enabledelayedexpansion
for /f %%t in ('dir /b/s win*.wim') do set menu=!menu! %%t ""
dialog --ok-label "FLASH..." --menu "Select OS Version for Flashing" 30 60 20 !menu:~1! 2>%TEMP%\ossel.txt
endlocal
for /f %%f in (%TEMP%\ossel.txt) do (
    set wim=%%~ff
    set rel=%%~pf
)
echo Image  : %wim%
echo Release: %rel%
dism /apply-image /imagefile:%wim% /index:1 /applydir:c:\
```

