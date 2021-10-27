﻿# Tera Term が使用するフォルダ

## デフォルトで使用するフォルダ

- 設定ファイル
  - %APPDATA%\teraterm5
- 動作ログ、ダンプ
  - %LOCALAPPDATA%\teraterm5
- 実行ファイル(32bitOS+32bitEXEの場合)
  - %PROGRAMFILES%\teraterm5

### Windows Vista以降 の場合のフォルダ例

- 設定ファイル
  - C:\Users\[usernane]\AppData\Roaming\teraterm5
- 動作ログ、ダンプ
  -  C:\Users\[usernane]\AppData\Local\teraterm5
- 実行ファイル(32bitOS+32bitEXE)
  - C:\Program Files\teraterm5

### Windows 2000, XP の場合のフォルダ例

- 設定ファイル
  - C:\Documents and Settings\[usernane]\AppData\Roaming\teraterm5
- 動作ログ、ダンプ(未定義)
  - C:\Documents and Settings\[usernane]\Application Data\teraterm5
- 実行ファイル
  - C:\Program Files\teraterm5

### Windows 2000より以前の場合のフォルダ例

- 設定ファイル
  - C:\My Documents\teraterm5
- 動作ログ、ダンプ(未定義)
  - C:\My Documents\teraterm5
- 実行ファイル
  - C:\Program Files\teraterm5

参考
- https://torutk.hatenablog.jp/entry/20110604/p1

## ファイルを指定した場合

コマンドラインでファイルをフルパスで指定した場合は、それを優先して使用する
- TERATERM.INI
- KEYBOARD.CNF
- broadcast.log


## 設定ファイルがない場合

次のように動作する
- 設定ファイルを置くフォルダを作成する
- ttermpro.exeのフォルダにある設定ファイルを
- 設定ファイルを置くフォルダにコピーする

