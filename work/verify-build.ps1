$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath (Split-Path -Parent $PSScriptRoot)
$engineCompiler = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231'
$engineSdk = 'C:\Program Files (x86)\Windows Kits\10'
$env:PATH = "$engineCompiler\bin\Hostx64\x64;$engineSdk\bin\10.0.26100.0\x64;" + $env:PATH
$env:INCLUDE = "$engineCompiler\include;$engineSdk\Include\10.0.26100.0\ucrt;$engineSdk\Include\10.0.26100.0\shared;$engineSdk\Include\10.0.26100.0\um;$engineSdk\Include\10.0.26100.0\winrt"
$env:LIB = "$engineCompiler\lib\x64;$engineSdk\Lib\10.0.26100.0\ucrt\x64;$engineSdk\Lib\10.0.26100.0\um\x64"
& 'C:\Program Files\CMake\bin\cmake.exe' --build build/verify --config Debug
if ($LASTEXITCODE -ne 0) { throw 'Debug build failed' }
& 'C:\Program Files\CMake\bin\cmake.exe' --build build/verify --config Release
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
