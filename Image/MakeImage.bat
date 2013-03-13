@echo off

set FloppySize=360
::set FloppySize=1440
::set ImageType=floppy
set ImageType=cdrom

echo Selected configuration: %ImageType%
if "%ImageType%"=="floppy" echo Floppy size: %FloppySize%k

if exist VortOS.iso del VortOS.iso
if exist VortOS.ima del VortOS.ima
copy OpenSans.vfnt ImageFiles\OpenSans.vfnt > nul
if "%ImageType%"=="floppy" (
  if exist ImageFiles\CDBoot.bin del ImageFiles\CDBoot.bin
  copy FlpBoot.bin ImageFiles\FlpBoot.bin > nul
  if "%FloppySize%"=="1440" (
    bfi -t=6 -f=VortOS.ima -l=Vort_OS -b=FlpBootSect1440.bin ImageFiles
  ) else (
    bfi -t=3 -f=VortOS.ima -l=Vort_OS -b=FlpBootSect360.bin ImageFiles
  )
  if not exist VortOS.ima exit /B -1
) else (
  if exist ImageFiles\FlpBoot.bin del ImageFiles\FlpBoot.bin
  copy CDBoot.bin ImageFiles\CDBoot.bin > nul
  mkisofs.exe -b CDBoot.bin -no-emul-boot -boot-load-size 4 -o VortOS.iso -iso-level 4 ImageFiles
  if not exist VortOS.iso exit /B -1
)