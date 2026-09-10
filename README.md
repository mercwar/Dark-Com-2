
<a target="_self" title="CLICK HERE to ENTER the GATEWAY FREE!" href="https://mercwar.github.io/Constellation/index.html">
<img 
    src="https://raw.githubusercontent.com/mercwar/Robo-Knight-Gallery/refs/heads/main/Version%207/image_d2a07390.png" 
    alt="Mercwar Constellation" 
    style="width:100%; height:auto; border-radius:12px; box-shadow:0 6px 16px rgba(0,0,0,0.6);"
/>
</a>

---


# ✨ Joe Tron's Dark-Com-2 WEB BROWSER 💎🔥
###### Now written in pure C !
- Same copy of the browser, with a diffferent source extension
- Just run Dark-Com-2-c.bat and it will compile from main.c instead of main.cpp
- 
- Compiled with the Official 100% FREE MSVC Build Tools - [Download](https://aka.ms/vs/17/release/vs_BuildTools.exe) From Microsoft
- C/CPP/EXE Version Dark-Com (Single File)
- True Native MS Windows Runtime (Win11 Ready)
- Official Mercwar Open Source Wrapper with Shell Installer
- CVBGOD's C/CPP file for the Official WebView2 Microsoft Device 
- Low cost CPU/RAM for Fast Browsing
- Navigation bar for regular surfing
- No Java/VB runtimes needed

This Uplink: The Official WebView2 `/packages` [Readme](packages/readme.md)

#



Requirements for Users:
- Run the Dark-Com-2.exe file after you clone the repo
- you can also run the net-installer and if you need the runtime the net-installer installs it for you

Requirements for Developers:
- Install MSVC Buiild Tools
- Clone the Dark-Com-2 repo / run net-install
- Run Install_MS_WebView2
- Run Dark-Com-2.bat
- Dark-Com-2.exe will auto Load
- There is a copy of WebView2.h in the clone, Install_MS_WebView2 does this for you
  
## 🎤 All in 1 C/CPP file, Your Done!



## ✨ *... And now the official Dark-Com-2 Readme*

![dc](images/ChatGPT%20Image%20Sep%204%2C%202026%2C%2006_51_00%20AM.png)

###### "<i>I am CVBGOD, and I have given it to you</i>!"
#

# ✨ Dark-Com-2 Stargate
## 1. Overview 💎🔥
Dark-Com-2 isn’t just another browser wrapper — it’s a **Win32-native beast**. Built directly on Microsoft WebView2 C/C++ controller architecture, it bypasses bloated runtimes and talks straight to the OS with hardware-accelerated messaging.  

Think of it as your **personal command gateway**: lean, fast, and unapologetically raw.

---

## 2. Features 💎🔥
- **Localized Cache Rooting** → No directory headaches, it auto-roots itself.  
- **Native Desktop Panel UI** → Ultra-fast orchestration with Win32 handles.  
- **Streamlined Deployment** → Zero config. Just run it.  
- **Unicode Compliance** → No string truncation nightmares.  
- **Baked-In Assets** → Icons and branding fused into the binary.  

---

## 3. Prerequisites 💎🔥
Before you dive in, make sure you’ve got:
- **WebView2 Runtime** (Win11: built-in, Win10: install Evergreen).  
- **VC++ 2015–2022 Redistributable** (x64).  

---
![dc](images/ChatGPT%20Image%20Sep%204%2C%202026%2C%2006_42_27%20AM.png)

## 4. Installation & Setup 💎🔥
1. Run `install_webview2.bat` → auto-fetches WebView2 assemblies.  
2. Run `Dark-Com-2.bat` → compiles everything into a standalone binary.  

---



## 5. File Infrastructure 💎🔥
```text
📁 Dark-Com-2/
│
├── 📁 src/                     <-- Source code
│   ├── 📄 main.cpp             <-- Core Win32 + WebView2 engine
│   ├── 📄 main.c               <-- Core Win32 + WebView2 engine
│   └── 📄 resource.rc          <-- Resource script
│
├── 📁 include/                 <-- Header files
│   └── 📄 WebView2.h           <-- WebView2 SDK header
│
├── 📁 assets/                  <-- Icons, branding, artwork
│   └── 📄 favi-con.ico
│
├── 📁 build/                   <-- Build outputs
│   ├── 📄 Dark-Com-2.exe       <-- Compiled binary
│   ├── 📄 resource.res         <-- Compiled resources
│   ├── 📄 main.obj             <-- Object file
│   └── 📄 WebView2Loader.dll   <-- Runtime loader
│
├── 📁 packages/                <-- External packages
│── 📁 browser_cache/           <-- Localized profile
│
├── 📁 scripts/                 <-- Batch installers & utilities
│   ├── 📄 Dark-Com-2.bat
│   ├── 📄 Dark-Com-2-c.bat
│   ├── 📄 Install_MS_WebView2.bat
│   ├── 📄 net-install.bat
│   └── 📄 nuget.exe
│
├── 📁 dist/                    <-- Distribution artifacts
│   └── 📄 Dark-Com-2.zip       <-- Packaged release
│
└── 📄 README.md                <-- Documentation
```

---

## 6. 🚀 Summary
Dark-Com-2 is designed for **instant deployment**. No configs, no headaches. Just run the batch files and watch it build itself into a glowing executable.

---

## 7. How it works 💎🔥
![dc](images/ChatGPT%20Image%20Sep%204%2C%202026%2C%2006_51_26%20AM.png)


- Dark-Com-2 is a Web browser that uses Microsoft Windows Webview (Edge) 
- It calls for Windows to provide an instance of the browser (WebView2Loader.dll)
- It keeps a copy of its records in the directory root of the application Dark-Com-2.exe file So you can log in and save your data normally
- You can always copy the browser_cache to any other folder you want to run the exe and use different sets of log in info
- Keep the name browser_cache copy to another folder with the exe and run files including WebView2Loader.dll
- You will see a new browser_cache folder in whatever browser you run (.exe), this log in info you save during your surf, will be saved independantly from other browser_cache folders
- Try saving the browser cache in a different folder and modifying the line 150 WinMain, this is where you name your directory.
- Create an ini file or json file with the names of your personal browser_cache folders compatible with this version of WebView2 .
- Now add a control on the form , a list box and load the list form the ini file
- Choose to select the current value of the list and that should be a directory name , now apply that variable ahead of creatwindow using a global
- That global should be restored form the previous session , so if you give your end user a button it says 'Apply' and apply is a section in the json file or a seperate file
- Now when the user restarts the application you load the recent press of the apply button to the global
- Now use that global char name the replacement for browser_cache
- You completed the mod and now your end user can select different <browser_cache> folders editing the ini/json file with any name, not only browser_cache
- Add a list box so the user can save to the ini file without using notpad, YOUR DONE!

  ```
      // Root the browser profile storage folder cleanly relative to the executable
    std::wstring cachePath = appRootDir + L"\\browser_cache";
  
  ```
---

## 8. Repository Cloning 💎🔥
Ready to get your hands dirty? Let’s make this interactive.  

👉 **Step 1: Clone the repo**  
```cmd
git clone https://github.com/mercwar/Dark-Com-2.git
```

👉 **Step 2: Run the one-click batch file**  
This script clones the repo **and** installs the VC++ Redistributable automatically.  
VC:
```bat
@echo off
echo ============================================
echo  AIFVS-ARTIFACT: Dark-Com-2.bat [C EDITION]
echo  Auto MSVC env + Compile WebView2 View Engine
echo ============================================

:: Force execution context to lock directly onto the script's home folder location
cd /d "%~dp0"

:: Flush previous operational application states and runtime cached objects
if exist Dark-Com-2.exe del /f /q Dark-Com-2.exe
if exist main.obj del /f /q main.obj
if exist resource.res del /f /q resource.res

:: Initialize local compiler dependencies if environment context paths are blank
if not defined VCINSTALLDIR (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else (
        echo ERROR: Visual Studio vcvarsall.bat environment initializer script not found.
        goto end
    )
)

:: Validate package folder structure before trying to target compilation paths
if not exist "packages\Microsoft.Web.WebView2" (
    echo ERROR: WebView2 NuGet dependency package is missing in 'packages\' directory.
    goto end
)

:: Verify presence of the base input main.c source architecture
if not exist main.c (
    echo ERROR: Base compilation target file 'main.c' is missing!
    goto end
)

echo Compiling embedded binary resource layout assets...
set "RES_FILE="
if exist favi-con.ico (
    if exist resource.rc (
        rc.exe resource.rc
        if exist resource.res set "RES_FILE=resource.res"
    ) else (
        echo WARNING: resource.rc missing. Skipping resource compilation step...
    )
) else (
    echo WARNING: favi-con.ico was missing! Skipping embedding step...
)

echo Building application window layer: Dark-Com-2.exe

:: Compile pure C code, moving the resource file down to the link stage parameters
cl ^
  main.c ^
  /DUNICODE /D_UNICODE /DCINTERFACE ^
  /I"packages\Microsoft.Web.WebView2\build\native\include" ^
  /Fe"Dark-Com-2.exe" ^
  /TC ^
  /link ^
  %RES_FILE% ^
  /LIBPATH:"packages\Microsoft.Web.WebView2\build\native\x64" ^
  WebView2Loader.dll.lib user32.lib ole32.lib gdi32.lib

if exist Dark-Com-2.exe (
    echo.
    echo Build Complete: Dark-Com-2.exe
    echo --------------------------------------------
    
    :: Safe extraction transfer routing of the required user engine system loader artifact
    if exist "packages\Microsoft.Web.WebView2\build\native\x64\WebView2Loader.dll" (
        copy /Y "packages\Microsoft.Web.WebView2\build\native\x64\WebView2Loader.dll" "WebView2Loader.dll" >nul
    )
    
    echo Launching Embedded Form Window Instance...
    Dark-Com-2.exe
) else (
    echo.
    echo --------------------------------------------
    echo ERROR: Build failed. Verify your compiler or asset paths.
    echo --------------------------------------------
)

:end
pause

```
VC++
```bat
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

```
![dc](images/ChatGPT%20Image%20Sep%204%2C%202026%2C%2005_12_02%20AM.png)
---

## 🎤 Cyborg Challenge
All in 1 C/CPP file !

- Clone the repo.  
- Run the batch files to install the enviorment.  
- Compile the C/cpp file
- Run the exe
- Then ...
- Get ready for AVIS!
---

## 7. Legal 💎🔥

Dark-Com-2 is released as a **public artifact** under open-source licensing. That means you’re free to explore, modify, and redistribute — but you do so at your own risk.  

- **Warranty Disclaimer:** All code, binaries, and documentation are provided **“AS IS”**. No guarantees, no promises — just raw engineering power.  
- **Liability Shield:** The authors and copyright holders are not responsible for damages, claims, or liabilities that may arise from using, misusing, or redistributing this software.  
- **Compliance Responsibility:** You, the user, are responsible for ensuring compliance with local laws and regulations when deploying or sharing Dark-Com-2.  
- **Trademark Notice:** **Microsoft WebView2** is a trademark of Microsoft Corporation. Its runtime components remain subject to Microsoft’s licensing terms.  

---

© 2026 – Mercwar. All Rights Reserved. **Public Artifact Edition.**  
