
<a target="_self" title="CLICK HERE to ENTER the GATEWAY FREE!" href="https://mercwar.github.io/Constellation/index.html">
<img 
    src="https://raw.githubusercontent.com/mercwar/Robo-Knight-Gallery/refs/heads/main/Version%207/image_d2a07390.png" 
    alt="Mercwar Constellation" 
    style="width:100%; height:auto; border-radius:12px; box-shadow:0 6px 16px rgba(0,0,0,0.6);"
/>
</a>

---


# ✨ Joe Tron's Dark-Com-2 WEB BROWSER 💎🔥
- Compiled with the Official 100% FREE MSVC Build Tools [Download from Microsoft](https://aka.ms/vs/17/release/vs_BuildTools.exe)
- CPP/EXE Version Dark-Com (Single File)
- True Native MS Windows Runtime (Win11 Ready)
- Official Mercwar Open Source Wrapper with Shell Installer
- CVBGOD's CPP file for the Official WebView2 Microsoft Device 
- Low cost CPU/RAM for Fast Browsing
- Navigation bar for regular surfing

  - The Free MSVC Compiler can be 
    


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
  
## 🎤 All in 1 .cpp file, Your Done!


##### "<i>I am CVBGOD, and I have given it to you...</i>"

#

## ✨ *...And now the official Dark-Com-2 Readme*


![dc](images/ChatGPT%20Image%20Sep%204%2C%202026%2C%2006_42_27%20AM.png)


## 1. Overview 💎🔥
Dark-Com-2 isn’t just another browser wrapper — it’s a **Win32-native beast**. Built directly on Microsoft WebView2 C++ controller architecture, it bypasses bloated runtimes and talks straight to the OS with hardware-accelerated messaging.  

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

## 4. Installation & Setup 💎🔥
1. Run `install_webview2.bat` → auto-fetches WebView2 assemblies.  
2. Run `Dark-Com-2.bat` → compiles everything into a standalone binary.  

---

![dc](images/ChatGPT%20Image%20Sep%204%2C%202026%2C%2006_51_26%20AM.png)

## 5. File Infrastructure 💎🔥
```text
📁 Dark-Com-2/
│
├── 📁 src/                     <-- Source code
│   ├── 📄 main.cpp             <-- Core Win32 + WebView2 engine
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

![dc](images/ChatGPT%20Image%20Sep%204%2C%202026%2C%2005_12_02%20AM.png)

- Dark-Com-2 is a Web browser that uses Microsoft Windows Webview (Edge) 
- It calls for Windows to provide an instance of the browser (WebView2Loader.dll)
- It keeps a copy of its records in the directory root of the application Dark-Com-2.exe file So you can log in and save your data normally
- You can always copy the browser_cache to any other folder you want to save your log in info
 
---

## 8. Repository Cloning 💎🔥
Ready to get your hands dirty? Let’s make this interactive.  

👉 **Step 1: Clone the repo**  
```cmd
git clone https://github.com/mercwar/Dark-Com-2.git
```

👉 **Step 2: Run the one-click batch file**  
This script clones the repo **and** installs the VC++ Redistributable automatically.  

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

---

## 🎤 Cyborg Challenge
All in 1 .cpp file !

- Clone the repo.  
- Run the batch files to install the enviorment.  
- Compile the cpp file
- run the exe
---

## 7. Legal 💎🔥

Dark-Com-2 is released as a **public artifact** under open-source licensing. That means you’re free to explore, modify, and redistribute — but you do so at your own risk.  

- **Warranty Disclaimer:** All code, binaries, and documentation are provided **“AS IS”**. No guarantees, no promises — just raw engineering power.  
- **Liability Shield:** The authors and copyright holders are not responsible for damages, claims, or liabilities that may arise from using, misusing, or redistributing this software.  
- **Compliance Responsibility:** You, the user, are responsible for ensuring compliance with local laws and regulations when deploying or sharing Dark-Com-2.  
- **Trademark Notice:** **Microsoft WebView2** is a trademark of Microsoft Corporation. Its runtime components remain subject to Microsoft’s licensing terms.  

---

© 2026 – mercwar. All Rights Reserved. **Public Artifact Edition.**  
