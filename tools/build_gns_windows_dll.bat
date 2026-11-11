@echo off
setlocal EnableDelayedExpansion
rem Builds GameNetworkingSockets v1.4.1 for Windows x64 with vcpkg and copies the DLLs into Unity.
set "LOG=%~dp0build_gns_windows_dll.log"
set "PLUGINS=%~dp0..\top-down-shooter\Assets\Plugins\x86_64"
set "VCPKG=C:\dev\vcpkg"
set "COMMIT=279bdb886599ba2007ca407f87ec6292bc85b34f"
set "VSI=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"

echo [%date% %time%] START > "%LOG%"

rem ---- 1. Visual Studio C++ tools + English language pack (vcpkg needs both)
set "VSCPP="
for /f "usebackq delims=" %%i in (`"%VSI%\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSCPP=%%i"
if not defined VSCPP (
  echo STEP 1: C++ tools missing - installing them. Accept the Windows admin prompt.
  echo STEP 1: C++ tools missing - installing >> "%LOG%"
  set "VSANY="
  for /f "usebackq delims=" %%i in (`"%VSI%\vswhere.exe" -latest -products * -property installationPath`) do set "VSANY=%%i"
  echo VS path: !VSANY! >> "%LOG%"
  "%VSI%\setup.exe" modify --installPath "!VSANY!" --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --addProductLang en-US --passive --norestart >> "%LOG%" 2>&1
  echo VS installer exit code: !errorlevel! >> "%LOG%"
  for /f "usebackq delims=" %%i in (`"%VSI%\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSCPP=%%i"
)
if not defined VSCPP ( echo FAILED: C++ tools still not installed >> "%LOG%" & goto :fail )
echo STEP 1 OK: C++ tools at !VSCPP! >> "%LOG%"

rem ---- 2. vcpkg pinned to the last commit with gamenetworkingsockets 1.4.1
if not exist "C:\dev" mkdir "C:\dev"
if not exist "%VCPKG%\.git" (
  echo STEP 2: cloning vcpkg >> "%LOG%"
  git clone https://github.com/microsoft/vcpkg.git "%VCPKG%" >> "%LOG%" 2>&1 || goto :fail
)
git -C "%VCPKG%" checkout %COMMIT% >> "%LOG%" 2>&1 || goto :fail
echo STEP 2 OK >> "%LOG%"

rem ---- 3. bootstrap
if not exist "%VCPKG%\vcpkg.exe" (
  call "%VCPKG%\bootstrap-vcpkg.bat" -disableMetrics >> "%LOG%" 2>&1 || goto :fail
)
echo STEP 3 OK >> "%LOG%"

rem ---- 4. build (20-40 min)
echo STEP 4: building gamenetworkingsockets - this takes 20-40 minutes >> "%LOG%"
echo Building... progress is in %LOG%
"%VCPKG%\vcpkg.exe" install gamenetworkingsockets:x64-windows >> "%LOG%" 2>&1 || goto :fail
echo STEP 4 OK >> "%LOG%"

rem ---- 5. copy DLLs into Unity
if not exist "%PLUGINS%" mkdir "%PLUGINS%"
copy /Y "%VCPKG%\installed\x64-windows\bin\*.dll" "%PLUGINS%\" >> "%LOG%" 2>&1 || goto :fail
dir /b "%PLUGINS%" >> "%LOG%"
echo [%date% %time%] DONE >> "%LOG%"
echo DONE - DLLs copied to %PLUGINS%
pause
exit /b 0

:fail
echo [%date% %time%] FAILED >> "%LOG%"
echo FAILED - see %LOG%
pause
exit /b 1
