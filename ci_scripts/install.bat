echo %~dp0\install.bat

cd /d %~dp0

setlocal
if "%CMAKE_COMMAND%" == "" (
   call find_cmake.bat
)

"%CMAKE_COMMAND%" -P ../buildtools/install_cygwin.cmake

if exist c:\msys64\usr\bin\pacman.exe (
  c:\msys64\usr\bin\pacman.exe  -S --noconfirm --needed cmake
)

rem ������ cd ���Ă��ړ�����Ȃ�
rem install.bat ���Ăяo�������Ŗ߂��Ă��炤
rem cd ..
