@echo off
setlocal
cd /d %~dp0
echo perl���_�E�����[�h����buildtools/perl�ɓW�J���܂�
pause
IF NOT EXIST "C:\Program Files\CMake\bin" goto by_powershell

:by_cmake
set PATH=C:\Program Files\CMake\bin;%PATH%
cmake -P getperl.cmake
goto finish

:by_powershell
powershell -NoProfile -ExecutionPolicy Unrestricted .\getperl.ps1
goto finish

:finish
endlocal
pause
