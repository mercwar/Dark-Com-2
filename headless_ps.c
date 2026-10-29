@echo off
:: InstallShield Subsystem: Force Elevated Administrative Token Context
:: -----------------------------------------------------------------------------
net session >nul 2>&1 || (powershell -Command "Start-Process '%~f0' -Verb RunAs" & exit /b)

setlocal DisableDelayedExpansion

:: =========================================================================
:: TARGET SYSTEM DIRECTORY MAPPING
:: =========================================================================
set "TARGET_BASE=C:\Program Files\dc2url\v1.0"
set "TARGET_DIR=%TARGET_BASE%\exec"
set "FINAL_EXE_PATH=%TARGET_DIR%\eclipse.exe"

if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"

title InstallShield (R) Setup Launcher v1.0 :: Eclipse Tray Master Deployment Engine
mode con: cols=120 lines=34

chcp 65001 >nul 2>&1
color 0E

:: Force TrueColor Console Canvas Context Allocation
reg add "HKCU\Console" /v VirtualTerminalLevel /t REG_DWORD /d 1 /f >nul 2>&1
for /f %%a in ('echo prompt $E^|cmd') do set "E=%%a"
set "RESET=%E%[0m"
set "BG_GLASS=%E%[48;2;10;10;12m"
set "O1=%E%[38;2;255;68;0m"
set "O2=%E%[38;2;255;110;0m"
set "O3=%E%[38;2;255;153;0m"
set "Y1=%E%[38;2;255;200;0m"
set "Y2=%E%[38;2;255;235;0m"
set "TXT=%E%[38;2;240;240;240m"

cls
echo %BG_GLASS%
cls

echo.
echo %O1%        ███████╗ ██████╗██╗     ██╗██████╗ ███████╗███████╗%RESET%%BG_GLASS%
echo %O2%        ██╔════╝██╔════╝██║     ██║██╔══██╗██╔════╝██╔════╝%RESET%%BG_GLASS%
echo %O3%        █████╗  ██║     ██║     ██║██████╔╝███████╗███████╗%RESET%%BG_GLASS%
echo %Y1%        ██╔══╝  ██║     ██║     ██║██╔═══╝ ╚════██║╚════██║%RESET%%BG_GLASS%
echo %Y2%        ███████╗╚██████╗███████╗██║██║     ███████║███████║%RESET%%BG_GLASS%
echo %Y2%        ╚══════╝ ╚═════╝╚══════╝╚═╝╚═╝     ╚══════╝╚══════╝%RESET%%BG_GLASS%
echo %O2%    ====================================================================================%RESET%%BG_GLASS%
echo %Y2%                    Eclipse Tray Cyborg Ecosystem Installer :: Mercwar AI%RESET%%BG_GLASS%
echo %O2%    ====================================================================================%RESET%%BG_GLASS%
echo.

:: 1. DOWNLINK ACQUISITION VIA DC2URL
echo %O2%        [*] Acquiring eclipse.exe via dc2url downlink engine...%RESET%%BG_GLASS%
dc2url "http://mercwar01.byethost3.com/darkcom2/downlink.php?file=eclipse/eclipse.exe" -o "%FINAL_EXE_PATH%"

if not exist "%FINAL_EXE_PATH%" (
    echo %O1%        [AVIS FAILED] dc2url execution failed to fetch or write the binary.%RESET%%BG_GLASS%
    goto :finish
) else (
    echo                      [SUCCESS] Downlink completed successfully.
)

:: 2. WINDOWS APP PATHS CONFIGURATION
echo %O2%        [*] Registering Eclipse App Paths into Registry...%RESET%%BG_GLASS%
reg add "HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\eclipse.exe" /f /ve /t REG_SZ /d "%FINAL_EXE_PATH%" >nul 2>&1
reg add "HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\eclipse.exe" /f /v "Path" /t REG_SZ /d "%TARGET_DIR%" >nul 2>&1

:: 3. CONTROL PANEL ADD/REMOVE PROGRAMS REGISTRATION
echo %O2%        [*] Injecting Windows Add/Remove Programs Metadata...%RESET%%BG_GLASS%
set "UNINSTALL_KEY=HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\eclipse"
reg add "%UNINSTALL_KEY%" /f /v "DisplayName" /t REG_SZ /d "Eclipse for Mercwar AI" >nul 2>&1
reg add "%UNINSTALL_KEY%" /f /v "UninstallString" /t REG_SZ /d "\"%TARGET_BASE%\uninstall.bat\"" >nul 2>&1
reg add "%UNINSTALL_KEY%" /f /v "Publisher" /t REG_SZ /d "Mercwar AI" >nul 2>&1
reg add "%UNINSTALL_KEY%" /f /v "EstimatedSize" /t REG_DWORD /d 250 >nul 2>&1
reg add "%UNINSTALL_KEY%" /f /v "NoModify" /t REG_DWORD /d 1 /f >nul 2>&1
reg add "%UNINSTALL_KEY%" /f /v "NoRepair" /t REG_DWORD /d 1 /f >nul 2>&1

:: 4. GENERATE CLEAN UNINSTALLER SCRIPT
echo %O2%        [*] Compiling local uninstaller payload...%RESET%%BG_GLASS%
(
echo @echo off
echo net session ^>nul 2^>&1 ^|^| ^(powershell -Command "Start-Process '%%~f0' -Verb RunAs" ^& exit /b^)
echo taskkill /f /im eclipse.exe ^>nul 2^>&1
echo rmdir /s /q "%TARGET_BASE%"
echo reg delete "HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\eclipse.exe" /f ^>nul 2^>&1
echo reg delete "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\eclipse" /f ^>nul 2^>&1
echo exit
) > "%TARGET_BASE%\uninstall.bat"

echo                      [SUCCESS] Ecosystem deployment pipeline finalized!
echo.

:finish
echo %TXT%        Setup complete. Launching eclipse.exe tray module...%RESET%
if exist "%FINAL_EXE_PATH%" (
    start "" "%FINAL_EXE_PATH%"
)
pause >nul
exit /b 0