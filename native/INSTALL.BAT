@echo off
copy native.exe %systemroot%\system32\.
regedit /s add.reg
echo Native Example Installed
