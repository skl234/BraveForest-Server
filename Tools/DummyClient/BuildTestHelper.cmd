@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
cl /nologo /EHsc /utf-8 /O2 /MT /Fo"Build\DummyClient\\" /Fe"Build\DummyClient\TestServerConsole.exe" "Tools\DummyClient\TestServerConsole.cpp"
popd
