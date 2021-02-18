@echo off

rem folder �́Apatch �����s���� .. ���猩�����΃p�X
set folder=openssl_patch

set cmdopt2=--binary --backup -p0
set cmdopt1=--dry-run %cmdopt2%


rem �p�b�`�R�}���h�̑��݃`�F�b�N
rem ..\%folder%\patch.exe, PATH���ʂ��Ă���patch �̗D�揇
pushd ..
set patchcmd="%folder%\patch.exe"
if exist %patchcmd% (
    popd
    goto cmd_true
)
popd

set patchcmd=patch
%patchcmd% -v
if %errorlevel% == 0 (goto cmd_true) else goto cmd_false

:cmd_true


:patch1
rem freeaddrinfo/getnameinfo/getaddrinfo API(WindowsXP�ȍ~)�ˑ������̂���
findstr /c:"# undef AI_PASSIVE" ..\openssl\crypto\bio\bio_local.h
if ERRORLEVEL 1 goto fail1
goto patch2
:fail1
pushd ..
%patchcmd% %cmdopt1% < %folder%\ws2_32_dll_patch2.txt
%patchcmd% %cmdopt2% < %folder%\ws2_32_dll_patch2.txt
popd

:patch2
:patch3
:patch4


:patch5
rem WindowsMe��RAND_bytes�ŗ����錻�ۉ���̂��߁B
rem OpenSSL 1.0.2�ł�meth��NULL�`�F�b�N�����������AOpenSSL 1.1.1�łȂ��Ȃ��Ă���B
rem ����NULL�`�F�b�N�͂Ȃ��Ă����͂Ȃ��A�{����InitializeCriticalSectionAndSpinCount�ɂ��邽�߁A
rem �f�t�H���g�ł͓K�p���Ȃ����̂Ƃ���B
rem findstr /c:"added if meth is NULL pointer" ..\openssl\crypto\rand\rand_lib.c
rem if ERRORLEVEL 1 goto fail5
rem goto patch6
rem :fail5
rem pushd ..
rem %patchcmd% %cmdopt1% < %folder%\RAND_bytes.txt
rem %patchcmd% %cmdopt2% < %folder%\RAND_bytes.txt
rem popd


:patch6
rem WindowsMe��InitializeCriticalSectionAndSpinCount���G���[�ƂȂ錻�ۉ���̂��߁B
findstr /c:"myInitializeCriticalSectionAndSpinCount" ..\openssl\crypto\threads_win.c
if ERRORLEVEL 1 goto fail6
goto patch7
:fail6
pushd ..
%patchcmd% %cmdopt1% < %folder%\atomic_api.txt
%patchcmd% %cmdopt2% < %folder%\atomic_api.txt
popd


:patch7
rem Windows98/Me/NT4.0�ł�CryptAcquireContextW�ɂ��G���g���s�[�擾��
rem �ł��Ȃ����߁A�V����������ǉ�����BCryptAcquireContextW�̗��p�͎c���B
findstr /c:"CryptAcquireContextA" ..\openssl\crypto\rand\rand_win.c
if ERRORLEVEL 1 goto fail7
goto patch8
:fail7
pushd ..
%patchcmd% %cmdopt1% < %folder%\CryptAcquireContextW2.txt
%patchcmd% %cmdopt2% < %folder%\CryptAcquireContextW2.txt
popd


:patch8
rem Windows95�ł� InterlockedCompareExchange �� InterlockedCompareExchange ��
rem ���T�|�[�g�̂��߁A�ʂ̏����Œu��������B
rem InitializeCriticalSectionAndSpinCount �����T�|�[�g�����AWindowsMe������
rem ���u�Ɋ܂܂��B
findstr /c:"INTERLOCKEDCOMPAREEXCHANGE" ..\openssl\crypto\threads_win.c
if ERRORLEVEL 1 goto fail8
goto patch9
:fail8
pushd ..
copy /b openssl\crypto\threads_win.c.orig openssl\crypto\threads_win.c.orig2
%patchcmd% %cmdopt1% < %folder%\atomic_api_win95.txt
%patchcmd% %cmdopt2% < %folder%\atomic_api_win95.txt
popd


rem Windows95�ł� CryptAcquireContextW �����T�|�[�g�̂��߁A�G���[�ŕԂ��悤�ɂ���B
rem �G���[��� CryptAcquireContextA ���g���B
:patch9
findstr /c:"myCryptAcquireContextW" ..\openssl\crypto\rand\rand_win.c
if ERRORLEVEL 1 goto fail9
goto patch10
:fail9
pushd ..
copy /b openssl\crypto\rand\rand_win.c.orig openssl\crypto\rand\rand_win.c.orig2
%patchcmd% %cmdopt1% < %folder%\CryptAcquireContextW_win95.txt
%patchcmd% %cmdopt2% < %folder%\CryptAcquireContextW_win95.txt
popd


:patch10


:patch_main_conf
rem �ݒ�t�@�C���̃o�b�N�A�b�v�����
if not exist "..\openssl\Configurations\10-main.conf.orig" (
    copy /y ..\openssl\Configurations\10-main.conf ..\openssl\Configurations\10-main.conf.orig
)

rem VS2005���ƌx���G���[�ŃR���p�C�����~�܂���ւ̏��u
perl -e "open(IN,'..\openssl\Configurations/10-main.conf');binmode(STDOUT);while(<IN>){s|/W3|/W1|;s|/WX||;print $_;}close(IN);" > conf.tmp
move conf.tmp ..\openssl\Configurations/10-main.conf

rem GetModuleHandleExW API(WindowsXP�ȍ~)�ˑ������̂���
perl -e "open(IN,'..\openssl\Configurations/10-main.conf');binmode(STDOUT);while(<IN>){s|(dso_scheme(.+)"win32")|#$1|;print $_;}close(IN);" > conf.tmp
move conf.tmp ..\openssl\Configurations/10-main.conf

rem Debug build��warning LNK4099�΍�(Workaround)
perl -e "open(IN,'..\openssl\Configurations/10-main.conf');binmode(STDOUT);while(<IN>){s|/Zi|/Z7|;s|/WX||;print $_;}close(IN);" > conf.tmp
move conf.tmp ..\openssl\Configurations/10-main.conf


:patch_end
echo "�p�b�`�͓K�p����Ă��܂�"
timeout 5
goto end


:patchfail
echo "�p�b�`���K�p����Ă��Ȃ��悤�ł�"
set /P ANS="���s���܂����H(y/n)"
if "%ANS%"=="y" (
  goto end
) else if "%ANS%"=="n" (
  echo "�o�b�`�t�@�C�����I�����܂�"
  exit /b
) else (
  goto fail
)

goto end

:cmd_false
echo �p�b�`�R�}���h��������܂���
echo ���L�T�C�g����_�E�����[�h���āA..\%folder% �� Git-x.xx.x-32-bit.tar.bz2 ����
echo patch.exe, msys-gcc_s-1.dll, msys-2.0.dll ��z�u���Ă�������
echo https://github.com/git-for-windows/git/releases/latest
echo.
goto patchfail

:end
@echo on


