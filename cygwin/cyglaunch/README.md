﻿# cyglaunch

cygterm を起動するプログラム

## コマンドラインオプション

cyglaunch で解釈されない引数は cygterm(又は msys2term)へ渡される

-msys2
    cygterm ではなく msys2term を起動する
    次のオプションを付けることで msys2の環境を設定できる

    -v MSYSTEM=MSYS
        msys
    -v MSYSTEM=MINGW32
        mingw32
    -v MSYSTEM=MINGW64
        mingw64

    msys2の起動については次のURLを参照
    https://www.msys2.org/wiki/Launchers/

## from explorer

レジストリに設定することで、エクスプローラーのコンテキストメニューからターミナルをオープンできる

cygterm_here_reg_sample.txt 参照

## 歴史

- cyglaunch は通常のexeファイル
- Visual Studio で普通にビルドできる
- cygwin上で動作する必要はない
- 従来は cygterm フォルダに存在し MinGW でビルドされていた
