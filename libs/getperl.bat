@echo off
setlocal
cd /d %~dp0
echo perl���_�E�����[�h����lib/perl�ɓW�J���܂�
pause
powershell -NoProfile -ExecutionPolicy Unrestricted .\getperl.ps1
endlocal
pause
