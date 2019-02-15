@echo off
if not exist build mkdir build

IF NOT DEFINED VisualStudioVersion (call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64)

set COMPILE_OPTIONS=-I..\\..\\external\\SDL2\\include -I..\\ -nologo /MTd /Zi /EHsc

set LINKER_OPTIONS=-subsystem:console -nodefaultlib:SDL2-static user32.lib gdi32.lib winmm.lib imm32.lib ole32.lib oleaut32.lib version.lib uuid.lib advapi32.lib shell32.lib SDL2-static.lib 

set SOURCES=..\\main.cpp 

pushd build
cl %COMPILE_OPTIONS% %SOURCES% /link %LINKER_OPTIONS% -out:mandelbrot.exe
popd