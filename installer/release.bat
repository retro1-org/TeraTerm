@echo off
setlocal
set CUR=%~dp0
cd /d %CUR%

set VS_VERSION=2019

if "%APPVEYOR%" == "True" set NOPAUSE=1

call :setup_tools_env

echo =======
echo 1. force download and rebuild libs / rebuild Tera Term, installer, archive
echo 2. download and build libs / rebuild Tera Term, installer, archive
echo 3. download and build libs / build Tera Term, installer, archive
echo 4. download and build libs
echo 5. build libs / rebuild Tera Term, installer, archive
echo 6. build libs / build Tera Term, installer, archive
echo 7. exec cmd.exe
echo 8. check tools
echo 9. exit

if "%1" == "" (
    set /p no="select no "
) else (
    set no=%1
)
echo %no%

if "%no%" == "1" (
    call :download_libs force
    call :build_libs
    call :build_teraterm rebuild
)

if "%no%" == "2" (
    call :download_libs
    call :build_libs
    call :build_teraterm rebuild
)

if "%no%" == "3" (
    call :download_libs
    call :build_libs
    call :build_teraterm
)

if "%no%" == "4" (
    call :download_libs
    call :build_libs
)

if "%no%" == "5" (
    call :build_teraterm rebuild
)

if "%no%" == "6" (
    call :build_teraterm
)

if "%no%" == "7" (
    call :exec_cmd
)

if "%no%" == "8" (
    call :check_tools
)

if not "%NOPAUSE%" == "1" pause
exit 0


rem ####################
:download_libs

setlocal
cd /d %CUR%..\libs

set OPT=
if "%1" == "force" set OPT=-DFORCE_DOWNLOAD=on
%CMAKE% %OPT% -P download.cmake

endlocal
exit /b 0

rem ####################
:build_libs

setlocal
cd /d %CUR%..\libs
call buildall.bat
endlocal
exit /b 0

rem ####################
:build_teraterm

setlocal
cd /d %CUR%
set TT_VERSION=
for /f "delims=" %%i in ('perl issversion.pl') do @set TT_VERSION=%%i

call ..\buildtools\svnrev\sourcetree_info.bat
if "%RELEASE%" == "1" (
    call build.bat rebuild
    call makearchive.bat release
) else if "%1" == "rebuild" (
    call build.bat rebuild
    call makearchive.bat
) else (
    call makearchive.bat
)
if not exist Output mkdir Output
set SNAPSHOT_PORTABLE_OUTPUT="teraterm-%TT_VERSION%-r%SVNVERSION%-%DATE%_%TIME%-%USERNAME%-snapshot"
if "%RELEASE%" == "1" (
    pushd Output
    %CMAKE% -E tar cf teraterm-%TT_VERSION%.zip --format=zip teraterm-%TT_VERSION%/
    popd
    set INNO_SETUP_OPT_VERSION=
    set INNO_SETUP_OPT_OUTPUT=
) else (
    %CMAKE% -E rename snapshot-%DATE%_%TIME% %SNAPSHOT_PORTABLE_OUTPUT%
    %CMAKE% -E tar cf Output/%SNAPSHOT_PORTABLE_OUTPUT%.zip --format=zip %SNAPSHOT_PORTABLE_OUTPUT%
    %CMAKE% -E rename %SNAPSHOT_PORTABLE_OUTPUT% snapshot-%DATE%_%TIME%
    set INNO_SETUP_OPT_VERSION="/DVerSubStr=r%SVNVERSION%-%DATE%_%TIME%"
    set INNO_SETUP_OPT_OUTPUT="/DOutputSubStr=r%SVNVERSION%-%DATE%_%TIME%-%USERNAME%-snapshot"
)
%INNO_SETUP% %INNO_SETUP_OPT_VERSION% %INNO_SETUP_OPT_OUTPUT% teraterm.iss

endlocal
exit /b 0

rem ####################
:setup_tools_env

set CURL=%SystemRoot%\System32\curl.exe
set CYGWIN_PATH=C:\cygwin64\bin
set VS_BASE=C:\Program Files (x86)\Microsoft Visual Studio\%VS_VERSION%

if exist toolinfo.bat (
    echo found toolinfo.bat
    call toolinfo.bat
    echo toolinfo.bat ok
) else (
    set PATH=
)

call :search_perl
call :search_svn
call :search_iscc
set PATH=%PATH%;%SVN_PATH%
set PATH=%PATH%;%PERL_PATH%
set PATH=%PATH%;%SystemRoot%
set PATH=%PATH%;%SystemRoot%\system32
call :set_vs_env
call :search_cmake
set PATH=%PATH%;%CYGWIN_PATH%
set PATH=%PATH%;%CMAKE_PATH%
exit /b 0

rem ####################
:search_perl
if exist %PERL_PATH%\perl.exe (
    set PERL=%PERL_PATH%\perl.exe
    exit /b 0
)

set PERL=perl.exe
where %PERL% > nul 2>&1
if %errorlevel% == 0 exit /b 0
set PERL=%CUR%..\buildtools\perl\perl\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\Strawberry\perl\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\Perl64\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\Perl\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\cygwin64\bin\perl.exe
if exist %PERL% exit /b 0
set PERL=C:\cygwin\bin\perl.exe
if exist %PERL% exit /b 0
echo perl not found
if not "%NOPAUSE%" == "1" pause
exit

rem ####################
:search_svn
if exist %SVN_PATH%\svn.exe (
    set SVN=%SVN_PATH%\svn.exe
    exit /b 0
)

set SVN=svn.exe
where %SVN% > nul 2>&1
if %errorlevel% == 0 exit /b 0
set SVN_PATH=C:\Program Files (x86)\Subversion\bin
set SVN="%SVN_PATH%\svn.exe"
if exist %SVN% exit /b 0
set SVN_PATH=C:\Program Files\TortoiseSVN\bin
set SVN="%SVN_PATH%\svn.exe"
if exist %SVN% exit /b 0
echo svn not found
if not "%NOPAUSE%" == "1" pause
exit

rem ####################
:search_cmake
if exist %CMAKE_PATH%\cmake.exe (
    set CMAKE="%CMAKE_PATH%\cmake.exe"
    exit /b 0
)

where %CMAKE% > nul 2>&1
if %errorlevel% == 0 exit /b 0
set CMAKE_PATH=C:\Program Files\CMake\bin
set CMAKE="%CMAKE_PATH%\cmake.exe"
if exist %CMAKE% exit /b 0
set CMAKE_PATH=%VCINSTALLDIR%\..\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin
set CMAKE="%CMAKE_PATH%\cmake.exe"
if exist %CMAKE% exit /b 0
echo cmake not found
if not "%NOPAUSE%" == "1" pause
exit

rem ####################
:search_iscc
if [%INNO_SETUP%] == [] goto search_iscc_1
if exist %INNO_SETUP% (
    exit /b 0
)
echo INNO_SETUP=%INNO_SETUP%
goto search_iscc_not_found

:search_iscc_1
set INNO_SETUP=%CUR%..\buildtools\innosetup6\bin\ISCC.exe
if exist %INNO_SETUP% exit /b 0
set INNO_SETUP="C:\Program Files (x86)\Inno Setup 6\iscc.exe"
if exist %INNO_SETUP% exit /b 0
:search_iscc_not_found
echo iscc(inno setup) not found
if not "%NOPAUSE%" == "1" pause
exit

rem ####################
:set_vs_env

if exist "%VS_BASE%\Community" (
    call "%VS_BASE%\Community\VC\Auxiliary\Build\vcvars32.bat"
    exit /b 0
)
if exist "%VS_BASE%\Professional" (
    call "%VS_BASE%\Profssional\VC\Auxiliary\Build\vcvars32.bat"
    exit /b 0
)
if exist "%VS_BASE%\Enterprise" (
    call "%VS_BASE%\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
    exit /b 0
)
:vs_not_found
echo Visual Studio not found
echo VS_BASE=%VS_BASE%
if not "%NOPAUSE%" == "1" pause
exit

rem ####################
:exec_cmd
call ..\buildtools\svnrev\svnrev.bat
call ..\buildtools\svnrev\sourcetree_info.bat
cmd
exit /b 0

rem ####################
:check_tools

echo cmd(windows)
ver

echo Visual Studio
echo VS_BASE=%VS_BASE%
cl

echo curl
where curl
echo CURL=%CURL%
%CURL% --version

echo svn
where svn
echo SVN_PATH=%SVN_PATH%
echo SVN=%SVN%
svn --version

echo perl
where perl
echo PERL_PATH=%PERL_PATH%
echo PERL=%PERL%
perl --version

echo cmake
where cmake
echo CMAKE_PATH=%CMAKE_PATH%
echo CMAKE=%CMAKE%
%CMAKE% --version

echo cygwin
echo CYGWIN_PATH=%CYGWIN_PATH%
cygcheck -c base-cygwin
cygcheck -c gcc-core
cygcheck -c w32api-headers
cygcheck -c make

echo inno setup
echo INNO_SETUP=%INNO_SETUP%
%INNO_SETUP% /?

exit /b 0
