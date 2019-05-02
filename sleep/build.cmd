set PROG=sleep

set INCLUDE=
set LIB=
set LIBPATH=
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
echo on
cl /Fe%PROG%-x64.exe %PROG%.c

set INCLUDE=
set LIB=
set LIBPATH=
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64_x86
echo on
cl /Fe%PROG%-x86.exe %PROG%.c

set INCLUDE=
set LIB=
set LIBPATH=
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64_arm
echo on
cl /D_ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE /Fe%PROG%-arm.exe %PROG%.c

pause
