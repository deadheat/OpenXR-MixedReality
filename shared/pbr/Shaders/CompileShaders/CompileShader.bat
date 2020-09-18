set SHADERC=C:\Users\t-ololu\source\repos\bgfx\.build\win32_vs2017\bin\shadercDebug
set SHADER_TMP=ShaderTemp.bin.h

pause
set VS_FLAGS=-i ./ --type vertex
set BASE_NAME=vs_bump_instanced
set TARGET_FILE=%BASE_NAME%.bin.h
pause
type nul > %TARGET_FILE%
%SHADERC% %VS_FLAGS% --debug -O 0 --platform windows -p vs_5_0 -O 3 -f %BASE_NAME%.sc -o %SHADER_TMP% 
copy /b %TARGET_FILE%+%SHADER_TMP% %TARGET_FILE%

pause

del %SHADER_TMP%
 