@echo off
:: CONFIGURATION - Replace with your actual absolute paths
set "SOURCE_FOLDER=C:\Path\To\Your\Local\Folder"
set "REPO_FOLDER=C:\Path\To\Your\Git\Repository"
set "COMMIT_MESSAGE=Automated file update"

echo Copying files from local folder to repo...
xcopy "%SOURCE_FOLDER%\*" "%REPO_FOLDER%\" /E /Y /I

echo Switching to repo directory...
cd /d "%REPO_FOLDER%"

echo Checking Git status...
git add .

echo Committing changes...
git commit -m "%COMMIT_MESSAGE%"

echo Pushing to remote repository...
git push

echo Done!
pause
