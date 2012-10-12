copy OpenSans.vfnt FloppyRoot\OpenSans.vfnt
copy FlpBoot.bin FloppyRoot\FlpBoot.bin
bfi -f=os.ima -l=Vort_OS -b=FlpBootSect.bin FloppyRoot

copy OpenSans.vfnt CDRoot\OpenSans.vfnt
copy CDBoot.bin CDRoot\CDBoot.bin
mkisofs.exe -b CDBoot.bin -no-emul-boot -boot-load-size 4 -o os.iso -iso-level 4 CDRoot