set outdir=Release
mkdir %outdir%
del %outdir%\* /Q

set linkparams=-nologo -nod -fixed -opt:ref -entry:Entry -subsystem:windows ..\my_memcpy.obj ..\my_memset.obj OpNewDel.obj String2.obj

cl.exe -c -GR- -GS- -Gs999999999 -Gy -Ox -Oy -O2 -Oi- -fp:fast -Fo"%outdir%\\" Hello.cpp Loader.cpp Test.cpp i8042.cpp PS2Keyb.cpp PS2Mouse.cpp Floppy.cpp DMA.cpp FileSys.cpp Init.cpp Viewer.cpp OpNewDel.cpp String2.cpp Cursor.cpp Font.cpp ProcInfo.cpp Desktop.cpp PCI.cpp Renderer.cpp SurfMgr.cpp DebugConsole.cpp Serial.cpp SerialMouse.cpp BochsVideo.cpp VMwareVideo.cpp VGAVideo.cpp PCIList.cpp ATA.cpp CLGD5446Video.cpp FAT.cpp Cache.cpp Partition.cpp Reboot.cpp SAnim.cpp FillBenchmark.cpp CDFS.cpp S3Trio64Video.cpp Benchmark.cpp
cd %outdir%
link.exe %linkparams% -out:S3Trio64Video.bi_ S3Trio64Video.obj
link.exe %linkparams% -out:CDFS.bi_ CDFS.obj
link.exe %linkparams% -out:FillBenchmark.bi_ FillBenchmark.obj
link.exe %linkparams% -out:Benchmark.bi_ Benchmark.obj
link.exe %linkparams% -out:SAnim.bi_ SAnim.obj
link.exe %linkparams% -out:Reboot.bi_ Reboot.obj
link.exe %linkparams% -out:Partition.bi_ Partition.obj
link.exe %linkparams% -out:Cache.bi_ Cache.obj
link.exe %linkparams% -out:Font.bi_ Font.obj
link.exe %linkparams% -out:Cursor.bi_ Cursor.obj
link.exe %linkparams% -out:Viewer.bi_ Viewer.obj
link.exe %linkparams% -out:Hello.bi_ Hello.obj
link.exe %linkparams% -out:Loader.bi_ Loader.obj
link.exe %linkparams% -out:Test.bi_ Test.obj
link.exe %linkparams% -out:i8042.bi_ i8042.obj
link.exe %linkparams% -out:PS2Mouse.bi_ PS2Mouse.obj
link.exe %linkparams% -out:PS2Keyb.bi_ PS2Keyb.obj
link.exe %linkparams% -out:Floppy.bi_ Floppy.obj
link.exe %linkparams% -out:DMA.bi_ DMA.obj
link.exe %linkparams% -out:FAT.bi_ FAT.obj
link.exe %linkparams% -out:FileSys.bi_ FileSys.obj
link.exe %linkparams% -out:Init.bi_ Init.obj
link.exe %linkparams% -out:ProcInfo.bi_ ProcInfo.obj ..\ulldiv.obj
link.exe %linkparams% -out:Desktop.bi_ Desktop.obj
link.exe %linkparams% -out:PCI.bi_ PCI.obj
link.exe %linkparams% -out:Renderer.bi_ Renderer.obj
link.exe %linkparams% -out:SurfMgr.bi_ SurfMgr.obj
link.exe %linkparams% -out:DebugConsole.bi_ DebugConsole.obj
link.exe %linkparams% -out:Serial.bi_ Serial.obj
link.exe %linkparams% -out:SerialMouse.bi_ SerialMouse.obj
link.exe %linkparams% -out:BochsVideo.bi_ BochsVideo.obj
link.exe %linkparams% -out:VMwareVideo.bi_ VMwareVideo.obj
link.exe %linkparams% -out:VGAVideo.bi_ VGAVideo.obj
link.exe %linkparams% -out:PCIList.bi_ PCIList.obj
link.exe %linkparams% -out:ATA.bi_ ATA.obj
link.exe %linkparams% -out:CLGD5446Video.bi_ CLGD5446Video.obj

..\DriverStripper.exe i8042.bi_ "|0|" "(0)"
..\DriverStripper.exe Serial.bi_ "|0|" "(0)"

..\DriverStripper.exe PS2Keyb.bi_ "|1|" "(0)"
..\DriverStripper.exe PS2Mouse.bi_ "|1|" "(0)"
..\DriverStripper.exe SerialMouse.bi_ "|1|" "(0)"

..\DriverStripper.exe Cursor.bi_ "|2|" "(1)"

..\DriverStripper.exe DMA.bi_ "|3|" "(0)"
..\DriverStripper.exe Floppy.bi_ "|3|" "(0)"
..\DriverStripper.exe FileSys.bi_ "|3|" "(1)"
..\DriverStripper.exe Partition.bi_ "|3|" "(0)"
..\DriverStripper.exe Cache.bi_ "|3|" "(0)"
..\DriverStripper.exe FAT.bi_ "|3|" "(0)"
..\DriverStripper.exe CDFS.bi_ "|3|" "(0)"
..\DriverStripper.exe PCI.bi_ "|3|" "(0)"
..\DriverStripper.exe ATA.bi_ "|3|" "(0)"

..\DriverStripper.exe Loader.bi_ "|4|" "(0)"
..\DriverStripper.exe Init.bi_ "|4|" "(0)"

..\DriverStripper.exe Hello.bi_ "|6|" "(2)"
..\DriverStripper.exe FillBenchmark.bi_ "|6|" "(2)"
..\DriverStripper.exe Benchmark.bi_ "|6|" "(2)"
..\DriverStripper.exe SAnim.bi_ "|6|" "(2)"
..\DriverStripper.exe PCIList.bi_ "|6|" "(2)"
..\DriverStripper.exe Viewer.bi_ "|6|" "(2)"
..\DriverStripper.exe ProcInfo.bi_ "|1|" "(2)"
..\DriverStripper.exe Reboot.bi_ "|6|" "(2)"
..\DriverStripper.exe DebugConsole.bi_ "|6|" "(2)"

..\DriverStripper.exe Font.bi_ "|7|" "(0)"
..\DriverStripper.exe Desktop.bi_ "|7|" "(1)"
..\DriverStripper.exe S3Trio64Video.bi_ "|8|" "(0)"
..\DriverStripper.exe BochsVideo.bi_ "|8|" "(0)"
..\DriverStripper.exe CLGD5446Video.bi_ "|8|" "(0)"
..\DriverStripper.exe VMwareVideo.bi_ "|8|" "(0)"
..\DriverStripper.exe VGAVideo.bi_ "|8|" "(0)"
..\DriverStripper.exe SurfMgr.bi_ "|8|" "(1)"
..\DriverStripper.exe Renderer.bi_ "|9|" "(0)"

..\DriverStripper.exe Test.bi_ "|10|" "(2)"

copy *.bin ..\..\Image\FloppyRoot\
copy *.bin ..\..\Image\CDRoot\

cd ..\..\Image\
call Pack.bat