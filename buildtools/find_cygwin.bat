rem output
rem     CYGWIN_PATH     cygwin��bin�t�H���_

set CYGWIN_PATH=%~dp0..\buildtools\cygwin64\bin
if exist "%CYGWIN_PATH%" exit /b 0
set CYGWIN_PATH=C:\cygwin64\bin
if exist "%CYGWIN_PATH%" exit /b 0
set CYGWIN_PATH=
exit /b 0
