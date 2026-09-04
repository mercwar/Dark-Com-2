
<a target="_self" title="CLICK HERE to ENTER the GATEWAY FREE!" href="https://mercwar.github.io/Constellation/index.html">
<img 
    src="https://raw.githubusercontent.com/mercwar/Robo-Knight-Gallery/refs/heads/main/Version%207/image_d2a07390.png" 
    alt="Mercwar Constellation" 
    style="width:100%; height:auto; border-radius:12px; box-shadow:0 6px 16px rgba(0,0,0,0.6);"
/>
</a>

---

# ✨ Joe Tron's Dark-Com-2 💎🔥

![dc](../images/ChatGPT%20Image%20Sep%204%2C%202026%2C%2006_51_00%20AM.png)

<!-- ============================================
     ⚡ Automated Installation via Batch Files
     ============================================ -->

<h1>⚡ Automated Installation via Batch Files</h1>

<p>
For developers who prefer automation, Dark‑Com‑2 includes several <strong>batch files</strong> to simplify setup. 
These scripts handle cloning the repository, installing prerequisites, and preparing the build environment. 
You can run them directly instead of following manual steps.
</p>

<hr/>

<h2>✅ Provided Batch Files</h2>
<ul>
  <li><strong>Dark-Com-2.bat</strong> → Compiles the project into a standalone executable.</li>
  <li><strong>Install_MS_WebView2.bat</strong> → Installs the Microsoft WebView2 Runtime automatically.</li>
  <li><strong>net-install.bat</strong> → Clones the repo, validates Git, checks VC++ Redistributable, and installs WebView2 if missing.</li>
</ul>

<hr/>

<h2>🚀 How to Use</h2>

<h3>Option A – Full Automated Install</h3>
<p>Run <code>net-install.bat</code> to perform a complete setup:</p>
<div style="background:#222;color:#0f0;padding:10px;border-radius:6px;">
<pre>
# Double-click net-install.bat
# Or run from command line:
E:\Dark-Com-2\net-install.bat
</pre>
</div>
<p>This script will:</p>
<ul>
  <li>Check if Git is installed.</li>
  <li>Clone the Dark‑Com‑2 repository.</li>
  <li>Validate VC++ Redistributable and install if missing.</li>
  <li>Install WebView2 Runtime if required.</li>
</ul>

<hr/>

<h3>Option B – Runtime Only</h3>
<p>Run <code>Install_MS_WebView2.bat</code> if you only need the WebView2 Runtime:</p>
<div style="background:#222;color:#0ff;padding:10px;border-radius:6px;">
<pre>
E:\Dark-Com-2\Install_MS_WebView2.bat
</pre>
</div>

<hr/>

<h3>Option C – Manual Build</h3>
<p>After prerequisites are installed, run <code>Dark-Com-2.bat</code> to compile:</p>
<div style="background:#222;color:#ff0;padding:10px;border-radius:6px;">
<pre>
E:\Dark-Com-2\Dark-Com-2.bat
</pre>
</div>

<hr/>

<h2>⚠️ Notes</h2>
<ul>
  <li>Batch files are designed for <strong>Windows environments</strong>.</li>
  <li>Administrator privileges may be required for runtime installation.</li>
  <li>If scripts fail, fall back to manual installation steps in the SDK + Runtime tutorial.</li>
</ul>

<hr/>

<h2>🎤 Summary</h2>
<p>
You can either <strong>install manually</strong using the SDK + Runtime tutorial, 
or <strong>use the provided batch files</strong for automated setup. 
Choose whichever method fits your workflow.
</p>


#
<!-- ============================================
     📖 Developer Tutorial: Installing WebView2 SDK + Runtime
     ============================================ -->

<h1>📖 Developer Tutorial: Installing Microsoft WebView2 SDK + Runtime</h1>

<p>
This tutorial is for <strong>developers building Dark‑Com‑2 from source</strong>. 
If you are only running the <code>Dark-Com-2.exe</code>, you do not need to install the SDK or Runtime.
</p>

<hr/>

<h2>✅ Step 1: Check Your Windows Version</h2>
<div style="background:#111;color:#0ff;padding:10px;border-radius:6px;">
Windows 11 → WebView2 Runtime is preinstalled.<br/>
Windows 10 → Must install Evergreen Runtime manually.
</div>

<hr/>

<h2>✅ Step 2: Install the WebView2 Runtime</h2>
<p>
Download the Evergreen Runtime from Microsoft:
<a href="https://developer.microsoft.com/en-us/microsoft-edge/webview2/" target="_blank">
WebView2 Runtime Download Page
</a>
</p>
<div style="background:#222;color:#0f0;padding:10px;border-radius:6px;">
<pre>
# Run the installer after download
vc_redist.x64.exe /install /quiet /norestart
</pre>
</div>
<p>
Evergreen auto‑updates silently in the background. Recommended for most developers and end users.
</p>

<hr/>

<h2>✅ Step 3: Install the WebView2 SDK</h2>
<p>
The SDK provides headers and libraries required to compile Dark‑Com‑2. 
You can install it via NuGet or manually download the SDK package.
</p>

<h3>Option A – NuGet (Recommended)</h3>
<div style="background:#222;color:#0ff;padding:10px;border-radius:6px;">
<pre>
# Visual Studio → Tools → NuGet Package Manager
PM> Install-Package Microsoft.Web.WebView2
</pre>
</div>

<h3>Option B – Manual SDK Download</h3>
<p>
Download the SDK from Microsoft:
<a href="https://developer.microsoft.com/en-us/microsoft-edge/webview2/#download-section" target="_blank">
WebView2 SDK Download Page
</a>
</p>
<div style="background:#222;color:#ff0;padding:10px;border-radius:6px;">
<pre>
# Place headers and libs into your project include/lib paths
Dark-Com-2\include\WebView2.h
Dark-Com-2\build\WebView2Loader.dll
</pre>
</div>

<hr/>

<h2>✅ Step 4: Configure MSVC</h2>
<p>
Ensure your Visual Studio project is set up to use the WebView2 SDK:
</p>
<div style="background:#222;color:#0f0;padding:10px;border-radius:6px;">
<pre>
# Add include path
Project → Properties → C/C++ → Additional Include Directories
E:\Dark-Com-2\include

# Add library path
Project → Properties → Linker → Additional Library Directories
E:\Dark-Com-2\build
</pre>
</div>

<hr/>

<h2>✅ Step 5: Verify Installation</h2>
<div style="background:#222;color:#0f0;padding:10px;border-radius:6px;">
<pre>
# Check Add/Remove Programs for "Microsoft Edge WebView2 Runtime"
# Compile and run Dark-Com-2
msbuild Dark-Com-2.sln /p:Configuration=Release
</pre>
</div>
<p>
If <code>Dark-Com-2.exe</code> launches without missing DLL errors, installation succeeded. 
If errors persist, rerun the Evergreen installer and confirm SDK paths in MSVC.
</p>

<hr/>

<h2>🚀 Summary</h2>
<ul>
  <li><strong>Runtime</strong> → Required for execution (Evergreen recommended).</li>
  <li><strong>SDK</strong> → Required for compilation (NuGet or manual download).</li>
  <li><strong>MSVC Config</strong> → Add include/lib paths for headers and DLLs.</li>
</ul>
<p>With both Runtime and SDK installed, Dark‑Com‑2 builds and runs cleanly.</p>

<!-- ============================================
     ⚖️ Legal Section
     ============================================ -->

<h1>⚖️ Legal Notice</h1>

<p>
Dark‑Com‑2 is distributed as a <strong>public artifact</strong> for educational and development purposes. 
By using, compiling, or redistributing this software, you agree to the following terms:
</p>

<hr/>

<h2>📜 License & Usage</h2>
<ul>
  <li>All source code and batch scripts are provided <strong>"AS IS"</strong> without warranty of any kind.</li>
  <li>You may modify and redistribute the code, but you are responsible for compliance with local laws.</li>
  <li>Commercial use requires adherence to applicable open‑source licensing terms.</li>
</ul>

<hr/>

<h2>🛡️ Liability Disclaimer</h2>
<p>
The authors and contributors are <strong>not liable</strong> for damages, claims, or losses arising from use, misuse, or redistribution of Dark‑Com‑2. 
This includes but is not limited to data loss, system instability, or security vulnerabilities.
</p>

<hr/>

<h2>📦 Third‑Party Components</h2>
<ul>
  <li><strong>Microsoft WebView2 SDK + Runtime</strong> are required dependencies. These remain subject to Microsoft’s licensing terms.</li>
  <li><strong>VC++ Redistributable</strong> is a Microsoft component and must be installed under its own license.</li>
</ul>

<hr/>

<h2>© Copyright</h2>
<p>
© 2026 mercwar. All Rights Reserved.  
Dark‑Com‑2 is released as a <strong>Public Artifact Edition</strong>.
</p>
