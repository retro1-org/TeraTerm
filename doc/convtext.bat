set TOSJIS=perl 2sjis.pl
set REF_E=en\html\reference
set REF_J=ja\html\reference
%TOSJIS% -i ..\libs\oniguruma\COPYING   -o %REF_E%\Oniguruma-LICENSE.txt -l unix
%TOSJIS% -i ..\libs\oniguruma\COPYING   -o %REF_J%\Oniguruma-LICENSE.txt -l unix
%TOSJIS% -i ..\libs\oniguruma\doc\RE    -o %REF_E%\RE.txt                -l unix
%TOSJIS% -i ..\libs\oniguruma\doc\RE.ja -o %REF_J%\RE.txt      -c utf8   -l unix
%TOSJIS% -i ..\libs\libressl\COPYING    -o %REF_E%\LibreSSL-LICENSE.txt  -l unix
%TOSJIS% -i ..\libs\libressl\COPYING    -o %REF_J%\LibreSSL-LICENSE.txt  -l unix
%TOSJIS% -i ..\libs\SFMT\LICENSE.txt    -o %REF_E%\SFMT-LICENSE.txt      -l unix
%TOSJIS% -i ..\libs\SFMT\LICENSE.txt    -o %REF_J%\SFMT-LICENSE.txt      -l unix
%TOSJIS% -i ..\cygwin\cygterm\COPYING   -o %REF_E%\CygTerm+-LICENSE.txt  -l unix
%TOSJIS% -i ..\cygwin\cygterm\COPYING   -o %REF_J%\CygTerm+-LICENSE.txt  -l unix
%TOSJIS% -i ..\libs\zlib\README         -o %REF_E%\zlib-LICENSE.txt      -l unix --zlib_special
%TOSJIS% -i ..\libs\zlib\README         -o %REF_J%\zlib-LICENSE.txt      -l unix --zlib_special
%TOSJIS% -i ..\libs\cJSON\LICENSE       -o %REF_E%\cJSON-LICENSE.txt     -l crlf
%TOSJIS% -i ..\libs\cJSON\LICENSE       -o %REF_J%\cJSON-LICENSE.txt     -l crlf
%TOSJIS% -i ..\libs\argon2\LICENSE      -o %REF_E%\argon2-LICENSE.txt    -l unix
%TOSJIS% -i ..\libs\argon2\LICENSE      -o %REF_J%\argon2-LICENSE.txt    -l unix

%TOSJIS% -i %REF_J%/build_with_cmake.md -o %REF_J%/build_with_cmake.html
%TOSJIS% -i %REF_E%/build_with_cmake.md -o %REF_E%/build_with_cmake.html
%TOSJIS% -i %REF_J%/build_library_with_cmake.md -o %REF_J%/build_library_with_cmake.html
%TOSJIS% -i %REF_E%/build_library_with_cmake.md -o %REF_E%/build_library_with_cmake.html

