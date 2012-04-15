set outdir=Release
del %outdir%\* /Q
cl.exe -c -GS- -GL -GF -Ox -Oy -O2 -Zi -Fo"%outdir%\\" /Fd"%outdir%\vc100.pdb" main.cpp Kernel.cpp PTE.cpp PDE.cpp PhysMemManager.cpp GDT.cpp Global.cpp OpNewDel.cpp TSS.cpp Task.cpp IDT.cpp Thread.cpp IntManager.cpp
link.exe -debug -nod -ltcg -opt:ref -fixed -base:0x20000 -entry:Entry -subsystem:windows -out:%outdir%\kernel.bi_ -filealign:4096 %outdir%\*.obj *.obj
cd %outdir%
..\KernelStripper.exe
copy Kernel.bin ..\..\Image\FloppyRoot
copy Kernel.bin ..\..\Image\CDRoot
cd ..\..\Image\
call Pack.bat