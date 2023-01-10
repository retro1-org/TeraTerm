﻿# Inno Setup

- https://jrsoftware.org/isinfo.php

- `cmake -P innosetup.cmake` で innosetup を buildtools/ に展開します

## Inno Setup をインストールせずに使用

- 'cmake -P innosetup.cmake' で innosetup を使えるよう準備する
- innosetupインストーラーから innosetup6/ を作成する
  - innosetupインストーラーを解凍するために innounp を使用
  - innounp を解凍するために unrar を使用
- buildtools/innosetup6/bin/ISCC.exe を使ってインストーラーを作成できる

## UnRAR

- https://www.rarlab.com/rar_add.htm

## innounp

the Inno Setup Unpacker

- https://innounp.sourceforge.net/
