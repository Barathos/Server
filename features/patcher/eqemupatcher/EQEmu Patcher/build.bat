@echo off
echo building...
if "%VERSION%"=="" set "VERSION=0.1.0.0"
if "%BUILD_CONFIGURATION%"=="" set "BUILD_CONFIGURATION=Release"
if "%BUILD_TARGET%"=="" set "BUILD_TARGET=Build"
if "%MSBUILD_VERBOSITY%"=="" set "MSBUILD_VERBOSITY=minimal"
if "%SERVER_NAME%"=="" set "SERVER_NAME=EQEmu Patcher"
if "%FILE_NAME%"=="" set "FILE_NAME=eqemupatcher"
if "%PATCH_BASE_URL%"=="" set "PATCH_BASE_URL=http://localhost:8080/patcher/"
if "%FILELIST_URL%"=="" set "FILELIST_URL=%PATCH_BASE_URL%"
if "%PATCHER_URL%"=="" set "PATCHER_URL=%PATCH_BASE_URL%"
if "%PATCH_NOTES_URL%"=="" set "PATCH_NOTES_URL=%PATCH_BASE_URL%patch_notes.txt"
if "%SERVICE_STATUS_URL%"=="" set "SERVICE_STATUS_URL=%PATCH_BASE_URL%patcher_status.yml"

set "MSBUILD_EXE=C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\msbuild.exe"
if not exist "%MSBUILD_EXE%" set "MSBUILD_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD_EXE%" (
  echo MSBuild was not found. Install Visual Studio Build Tools on the build VM.
  exit /b 1
)
if not exist "%ProgramFiles(x86)%\Reference Assemblies\Microsoft\Framework\.NETFramework\v4.8" (
  echo .NET Framework 4.8 Developer Pack is required to build eqemupatcher.
  echo Install the targeting pack on the build VM, then rerun this script.
  exit /b 1
)

if not exist "packages\Fody.2.1.0\build\netstandard1.0\Fody.targets" (
  echo Restoring NuGet packages...
  "%MSBUILD_EXE%" "EQEmu Patcher.sln" /t:Restore /p:RestorePackagesConfig=true /v:%MSBUILD_VERBOSITY%
  if errorlevel 1 exit /b 1
)

"%MSBUILD_EXE%" /m /v:%MSBUILD_VERBOSITY% /t:%BUILD_TARGET% /p:Configuration=%BUILD_CONFIGURATION% /p:VERSION=%VERSION% /p:SERVER_NAME="%SERVER_NAME%" /p:FILELIST_URL="%FILELIST_URL%" /p:PATCHER_URL="%PATCHER_URL%" /p:PATCH_NOTES_URL="%PATCH_NOTES_URL%" /p:SERVICE_STATUS_URL="%SERVICE_STATUS_URL%" /p:FILE_NAME="%FILE_NAME%"
