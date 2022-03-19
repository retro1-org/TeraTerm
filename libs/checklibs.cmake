﻿# ライブラリのバージョンをチェックする
#   installer/release.batから使用
# 単体で実行
#   cmake -P checklibs.cmake

# 全体のhashはこの値になる
set(EXPECT_HASH "8a9fc9f6658191cf42e678e0d1fdd42b7046285fb4ade64d566fb32430387285")

# バージョンをチェックするために使用するファイル
set(CHECK_VERSION_FILES
  cJSON/CHANGELOG.md
  libressl/ChangeLog
  oniguruma/HISTORY
  putty/version.h
  SFMT/CHANGE-LOG.txt
  zlib/ChangeLog
  )

# 各ファイルのhash
execute_process(
  COMMAND ${CMAKE_COMMAND} -E sha256sum ${CHECK_VERSION_FILES}
  OUTPUT_VARIABLE OV
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )
#message("${OV}")

# 全体のhash
string(SHA256 ACTUAL_HASH ${OV})

# 結果
if(${ACTUAL_HASH} STREQUAL ${EXPECT_HASH})
  message("ok")
  file(WRITE "checklibs_result.bat" "set RESULT=1\r\n")
else()
  message("ng")
  message("ACTUAL_HASH=${ACTUAL_HASH}")
  message("EXPECT_HASH=${EXPECT_HASH}")
  file(WRITE "checklibs_result.bat" "set RESULT=0\r\n")
endif()
