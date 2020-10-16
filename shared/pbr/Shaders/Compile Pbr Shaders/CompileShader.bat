set SHADERC=..\..\..\ext\bgfx\.build\win64_vs2019\bin\shadercDebug
set SHADER_TMP=ShaderTemp.bin.h
set OUTPUT_FLDR=..\Compiled


set VS_FLAGS=-i ./ --type vertex
set BASE_NAME=vs_PbrVertexShader
set TARGET_FILE=%OUTPUT_FLDR%\%BASE_NAME%.bin.h

type nul > %TARGET_FILE%
%SHADERC% %VS_FLAGS% --platform linux                  -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_glsl
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
%SHADERC% %VS_FLAGS% --platform linux   -p spirv       -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_spv
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
%SHADERC% %VS_FLAGS% --platform windows -p vs_5_0 -O 3 -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_dx9
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
%SHADERC% %VS_FLAGS% --debug -O 0 --platform windows -p vs_5_0 -O 3 -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_dx11
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
REM ECHO static const uint8_t vs_PbrVertexShader_glsl[1] = {0x00};>>%TARGET_FILE%
pause

set FS_FLAGS=-i ./ --type fragment
set BASE_NAME=fs_PbrPixelShader
set TARGET_FILE=%OUTPUT_FLDR%\%BASE_NAME%.bin.h

type nul > %TARGET_FILE%
%SHADERC% %FS_FLAGS% --platform linux                  -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_glsl
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
%SHADERC% %FS_FLAGS% --platform linux   -p spirv       -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_spv
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
%SHADERC% %FS_FLAGS% --platform windows -p ps_5_0 -O 3 -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_dx9
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
%SHADERC% %FS_FLAGS% --debug -O 0 --platform windows -p ps_5_0 -O 3 -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_dx11
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
%SHADERC% %FS_FLAGS%--platform ios     -p metal  -O 3  -f %BASE_NAME%.sc -o %SHADER_TMP% --bin2c %BASE_NAME%_mtl
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%
REM ECHO static const uint8_t fs_PbrPixelShader_glsl[1] = {0x00};>>%TARGET_FILE%

del %SHADER_TMP%
pause