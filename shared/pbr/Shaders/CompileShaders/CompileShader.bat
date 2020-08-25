set SHADERC=C:\Users\t-ololu\source\repos\bgfx\.build\win32_vs2017\bin\shadercDebug
set SHADER_TMP=ShaderTemp.bin.h

pause
set FS_FLAGS=-i ./ --type fragment
set BASE_NAME=fs_HighlightPixelShader
set TARGET_FILE=%BASE_NAME%.bin.h
pause
type nul > %TARGET_FILE%
%SHADERC% %FS_FLAGS% --platform linux                  -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_glsl
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
%SHADERC% %FS_FLAGS% --platform linux   -p spirv       -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_spv
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
%SHADERC% %FS_FLAGS% --platform windows -p ps_5_0 -O 3 -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_dx9
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
%SHADERC% %FS_FLAGS% --platform windows -p ps_5_0 -O 3 -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_dx11
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
%SHADERC% %FS_FLAGS%--platform ios     -p metal  -O 3  -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_mtl
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
pause

del %SHADER_TMP%
