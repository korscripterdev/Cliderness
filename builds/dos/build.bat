@echo off
rem Cliderness - Open Watcom DOS build for Windows / DOS hosts (needs WATCOM set)
rem Created by korscripterdev.  Run from the repository root:  builds\dos\build.bat
if defined WATCOM set INCLUDE=%WATCOM%\h
if not exist builds\dos\obj16 mkdir builds\dos\obj16
echo system dos> builds\dos\obj16\link.lnk
echo name builds\dos\CLIDE16.EXE>> builds\dos\obj16\link.lnk
echo option quiet>> builds\dos\obj16\link.lnk
echo option stack=24k>> builds\dos\obj16\link.lnk
for /f "eol=# delims=" %%s in (builds\dos\sources.lst) do (
  wcc -ml -zt=2048 -bt=dos -ox -zq -w3 -i=src -fo=builds\dos\obj16\%%~ns.obj %%s || goto fail
  echo file builds\dos\obj16\%%~ns.obj>> builds\dos\obj16\link.lnk
)
wlink @builds\dos\obj16\link.lnk || goto fail
echo Built builds\dos\CLIDE16.EXE
goto end
:fail
echo DOS build failed.
exit /b 1
:end
