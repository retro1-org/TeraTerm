rem OpenSSL�̃r���h

cd openssl


rem Visual Studio 2005 �̏ꍇ�̓p�b�`��K�p
set CL_VER=
for /f "delims=" %%o in ('cl 2^>^&1') do set CL_VER=%%o & goto end_clver_chk
:end_clver_chk

echo %CL_VER% | find "Compiler Version 14" >nul
if ERRORLEVEL 1 goto patch_end
pushd ..\openssl_patch
call apply_patch.bat
if ERRORLEVEL 1 (
    echo "buildopenssl11.bat ���I�����܂�"
    exit /b 1
)
popd

:patch_end


if exist "out32.dbg\libcrypto.lib" goto build_dbg_end
perl Configure no-asm no-async no-shared no-capieng no-dso no-engine VC-WIN32 -D_WIN32_WINNT=0x0501 --debug
perl -e "open(IN,'makefile');while(<IN>){s| /MDd| /MTd|;print $_;}close(IN);" > makefile.tmp
if exist "makefile.dbg" del makefile.dbg
ren makefile.tmp makefile.dbg
nmake -f makefile.dbg clean
nmake -f makefile.dbg
mkdir out32.dbg
move libcrypto* out32.dbg
move libssl* out32.dbg
move apps\openssl.exe out32.dbg
:build_dbg_end

if exist "out32\libcrypto.lib" goto build_end
perl Configure no-asm no-async no-shared no-capieng no-dso no-engine VC-WIN32 -D_WIN32_WINNT=0x0501
perl -e "open(IN,'makefile');while(<IN>){s| /MD| /MT|;print $_;}close(IN);" > makefile.tmp
if exist "makefile" del makefile
ren makefile.tmp makefile
nmake clean
nmake
mkdir out32
move libcrypto* out32
move libssl* out32
move apps\openssl.exe out32
:build_end


rem Visual Studio 2005 �̏ꍇ�� 2003 R2 Platform SDK �̓������m�F����
echo %CL_VER% | find "Compiler Version 14" >nul
if ERRORLEVEL 1 goto end
@echo off
if not exist out32\openssl.exe (
    echo crypt32.lib �������N�ł����Ƀo�C�i�����쐬�ł��Ă��܂���B
    echo Platform SDK����������Ă��Ȃ��\��������܂��B
    set /P ANS2003SDK="���s���܂����H(y/n)"
    if "%ANS2003SDK%"=="y" (
        goto end
    ) else (
        echo "buildopenssl11.bat ���I�����܂�"
        exit /b 1
    )
) else (
    echo OpenSSL�̃r���h������I�����܂����B
)
@echo on


:end
cd ..
