:: ============================================
:: AIFVS-ARTIFACT
:: File: net-install.bat
:: AVIS Neon Artifact - End-User Root Installer
:: ============================================

@echo off
setlocal

echo [INFO] Starting Dark-Com-2 net-install...
echo --------------------------------------------

:: --- Root to script directory ---
cd /d "%~dp0"
echo [INFO] Rooted working directory: %cd%

:: --- Validate Git installation ---
echo [INFO] Checking for Git...
where git >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Git is not installed or not in PATH.
    echo Please install Git from: https://git-scm.com/download/win
    pause
    exit /b 1
) else (
    git --version
    echo [SUCCESS] Git detected.
)

:: --- Clone Dark-Com-2 Repo ---
set REPO_DIR=%cd%\Dark-Com-2
if exist "%REPO_DIR%" (
    echo [WARN] Dark-Com-2 folder already exists. Skipping clone.
) else (
    echo [INFO] Cloning Dark-Com-2 repository...
    git clone https://github.com/mercwar/Dark-Com-2.git "%REPO_DIR%"
    if %errorlevel% neq 0 (
        echo [ERROR] Git clone failed.
        pause
        exit /b 1
    )
)
:: --- Validate VC++ Redistributable ---
echo [INFO] Checking if VC++ Redistributable (x64) is installed...

set "VCREG_KEY=HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64"

reg query "%VCREG_KEY%" /v Installed >nul 2>&1
if %errorlevel% equ 0 (
    echo [SUCCESS] VC++ Redistributable detected.
) else (
    echo [WARN] VC++ Redistributable not found. Preparing to download and install...
    set REDIST_URL=https://aka.ms/vs/17/release/vc_redist.x64.exe
    set REDIST_EXE=%cd%\vc_redist.x64.exe

    powershell -Command "Invoke-WebRequest -Uri %REDIST_URL% -OutFile '%REDIST_EXE%'"
    if not exist "%REDIST_EXE%" (
        echo [ERROR] Failed to download VC++ Redistributable.
        pause
        exit /b 1
    )

    echo [INFO] Installing VC++ Redistributable silently...
    "%REDIST_EXE%" /install /quiet /norestart
    if %errorlevel% neq 0 (
        echo [ERROR] VC++ Redistributable installation failed.
        pause
        exit /b 1
    )
    echo [SUCCESS] VC++ Redistributable installed successfully.
)

echo --------------------------------------------
echo [SUCCESS] Dark-Com-2 repo cloned and VC++ Redistributable validated!
echo Root directory: %REPO_DIR%
echo Next step: Run Dark-Com-2.bat inside the Dark-Com-2 folder to compile the engine.
echo --------------------------------------------

endlocal
pause
exit /b 0
