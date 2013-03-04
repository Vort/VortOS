@echo off

set ImageType=cdrom
::set ImageType=floppy

echo Selected configuration: %ImageType%

if exist VortOS.iso del VortOS.iso
if exist VortOS.ima del VortOS.ima
copy OpenSans.vfnt ImageFiles\OpenSans.vfnt > nul
if "%ImageType%"=="cdrom" goto cdrom
  if exist ImageFiles\CDBoot.bin del ImageFiles\CDBoot.bin
  copy FlpBoot.bin ImageFiles\FlpBoot.bin > nul
  bfi -f=VortOS.ima -l=Vort_OS -b=FlpBootSect.bin ImageFiles
  if not exist VortOS.ima exit /B -1
goto end
:cdrom
  if exist ImageFiles\FlpBoot.bin del ImageFiles\FlpBoot.bin
  copy CDBoot.bin ImageFiles\CDBoot.bin > nul
  mkisofs.exe -b CDBoot.bin -no-emul-boot -boot-load-size 4 -o VortOS.iso -iso-level 4 ImageFiles
  if not exist VortOS.iso exit /B -1
:end