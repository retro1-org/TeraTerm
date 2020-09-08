call svnrev_perl\svnrev.bat
call svnrev_perl\sourcetree_info.bat
if "%GENERATOR%" == "Visual Studio 8 2005" (
  cd buildtools
  call getcmake.bat nopause
  cd ..
)
if "%COMPILER%" == "mingw"  (
  set PATH=C:\msys64\mingw32\bin;C:\msys64\usr\bin
  pacman -S --noconfirm --needed mingw32/mingw-w64-i686-cmake
  if "%MINGW_CC%" == "clang" (
    pacman -S --noconfirm --needed mingw32/mingw-w64-i686-clang
  )
  set CC=%MINGW_CC%
  set CXX=%MINGW_CXX%
  set CMAKE_OPTION_BUILD=-- -s -j
  set CMAKE_OPTION_GENERATE=%CMAKE_OPTION_GENERATE% -DCMAKE_BUILD_TYPE=Release
)
if "%COMPILER%" == "mingw_x64"  (
  set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin
  pacman -S --noconfirm --needed mingw64/mingw-w64-x86_64-cmake
  if "%MINGW_CC%" == "clang" (
    pacman -S --noconfirm --needed mingw64/mingw-w64-x86_64-clang
  )
  set CC=%MINGW_CC%
  set CXX=%MINGW_CXX%
  set CMAKE_OPTION_BUILD=-- -s -j
  set CMAKE_OPTION_GENERATE=%CMAKE_OPTION_GENERATE% -DCMAKE_BUILD_TYPE=Release
)
cd libs
if not exist openssl11_%COMPILER% (
  "%CMAKE_COMMAND%" -DCMAKE_GENERATOR="%GENERATOR%" %CMAKE_OPTION_LIBS% -P buildall.cmake
  if exist build rmdir /s /q build
  if exist download rmdir /s /q download
  if exist openssl_%COMPILER%\html rmdir /s /q openssl_%COMPILER%\html
  if exist openssl_%COMPILER%_debug\html rmdir /s /q openssl_%COMPILER%_debug\html
  if exist ..\buildtools\perl\c rmdir /s /q ..\buildtools\perl\c
  if exist ..\buildtools\download rmdir /s /q ..\buildtools\download
)
cd ..
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%
if exist cmakecache.txt del cmakecache.txt
set ZIP_FILE=snapshot-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%.zip
set SETUP_FILE=snapshot-%VERSION%-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
set SNAPSHOT_DIR=snapshot-r%SVNVERSION%-%DATE%_%TIME%-appveyor-%COMPILER_FRIENDLY%
"%CMAKE_COMMAND%" .. -G "%GENERATOR%" %CMAKE_OPTION_GENERATE% -DSNAPSHOT_DIR=%SNAPSHOT_DIR% -DSETUP_ZIP=%ZIP_FILE% -DSETUP_EXE=%SETUP_FILE% -DSETUP_RELEASE=%RELEASE%
"%CMAKE_COMMAND%" --build . --target install %CMAKE_OPTION_BUILD%
"%CMAKE_COMMAND%" --build . --target zip
if NOT "%COMPILER%" == "mingw" if NOT "%COMPILER%" == "mingw_x64" (
  "%CMAKE_COMMAND%" --build . --target inno_setup
)
cd ..
