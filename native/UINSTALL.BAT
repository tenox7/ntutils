@echo off
del %systemroot%\system32\native.exe
regedit /s remove.reg
echo Native Example Uninstalled
