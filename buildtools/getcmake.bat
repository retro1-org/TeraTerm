@echo off
setlocal
cd /d %~dp0
IF NOT EXIST cmake-3.11.4-win32-x86 (
  echo cmake���_�E�����[�h����lib/cmake�ɓW�J���܂�
  IF NOT "%1" == "nopause" pause
  powershell -NoProfile -ExecutionPolicy Unrestricted .\getcmake.ps1
)
endlocal
IF NOT "%1" == "nopause" pause
