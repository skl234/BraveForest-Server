@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
if not exist "Build\DummyClient" mkdir "Build\DummyClient"
if not exist "MMOServerExes\DummyClient\Data" mkdir "MMOServerExes\DummyClient\Data"
cl /nologo /std:c++17 /EHsc /utf-8 /O2 /MT /W4 /DWIN32_LEAN_AND_MEAN /DNOMINMAX /Fo"Build\DummyClient\\" /Fe"MMOServerExes\DummyClient\DummyClient.exe" "Tools\DummyClient\main.cpp" "Tools\DummyClient\DummyClient.cpp" "Tools\DummyClient\CellMap.cpp" /link Ws2_32.lib
if errorlevel 1 (popd & exit /b 1)
copy /Y "Data\TownField.txt" "MMOServerExes\DummyClient\Data\TownField.txt" >nul
copy /Y "Data\BeginnerZoneField.txt" "MMOServerExes\DummyClient\Data\BeginnerZoneField.txt" >nul
popd
