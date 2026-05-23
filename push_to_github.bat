@echo off
setlocal

REM === Cau hinh repo GitHub ===
set "REPO_URL=https://github.com/mountianVn/PlayStation-Controller-To-CRSF.git"
set "BRANCH=main"

echo.
echo [1/6] Kiem tra Git...
git --version >nul 2>&1
if errorlevel 1 (
  echo Loi: Chua cai Git hoac Git khong nam trong PATH.
  pause
  exit /b 1
)

echo [2/6] Khoi tao repository neu chua co...
if not exist ".git" (
  git init
  if errorlevel 1 goto :fail
)

echo [3/6] Dat branch mac dinh la %BRANCH%...
git branch -M %BRANCH%
if errorlevel 1 goto :fail

echo [4/6] Cau hinh remote origin...
git remote get-url origin >nul 2>&1
if errorlevel 1 (
  git remote add origin %REPO_URL%
  if errorlevel 1 goto :fail
) else (
  git remote set-url origin %REPO_URL%
  if errorlevel 1 goto :fail
)

echo [5/7] Add toan bo file va commit...
git add -A
if errorlevel 1 goto :fail

git diff --cached --quiet
if errorlevel 1 (
  git commit -m "Update project files"
  if errorlevel 1 goto :fail
) else (
  echo Khong co thay doi moi de commit.
)

echo [6/7] Dong bo remote truoc khi push...
git pull --rebase --allow-unrelated-histories origin %BRANCH%
if errorlevel 1 (
  echo.
  echo Loi khi pull/rebase. Co the dang bi conflict.
  echo Mo file conflict, sua xong chay lai script.
  pause
  exit /b 1
)

echo [7/7] Day len GitHub...
git push -u origin %BRANCH%
if errorlevel 1 goto :fail

echo.
echo Hoan tat: Da day file len %REPO_URL%
pause
exit /b 0

:fail
echo.
echo Loi trong qua trinh thuc hien. Kiem tra thong bao tren man hinh.
pause
exit /b 1
