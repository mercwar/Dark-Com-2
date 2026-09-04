:: ============================================
:: AIFVS-ARTIFACT
:: File: net-install.bat
:: AVIS Neon Artifact - Repo Clone + Redist
:: ============================================

@echo off
setlocal

echo [INFO] Cloning Dark-Com-2 repository...
git clone https://github.com/mercwar/Dark-Com-2.git

echo [INFO] Downloading VC++ Redistributable...
set REDIST_URL=https://aka.ms/vs/17/release/vc_redist.x64.exe
set REDIST_EXE=vc_redist.x64.exe

powershell -Command "Invoke-WebRequest -Uri %REDIST_URL% -OutFile %REDIST_EXE%"
%REDIST_EXE% /install /quiet /norestart

echo [SUCCESS] Repo cloned + Redist installed!
echo Next: Run Dark-Com-2.bat to compile the engine.

endlocal
exit /b 0
