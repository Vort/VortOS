; ===============================================
org 0x7C00
use16

; -----------------------------------------------
EP:
   jmp	 Start
   nop

; -----------------------------------------------
   OEMname	       db 'VORT_OS '
   bytesPerSector      dw 0x0200
   sectPerCluster      db 0x01
   reservedSectors     dw 0x0001
   numFAT	       db 0x02
   numRootDirEntries   dw 0x00e0
   numSectors	       dw 0x0b40
   mediaType	       db 0xf0
   numFATsectors       dw 0x0009
   sectorsPerTrack     dw 0x0012
   numHeads	       dw 0x0002
   numHiddenSectors    dd 0x00000000
   numSectorsHuge      dd 0x00000000
   driveNum	       db 0x00
   reserved	       db 0x00
   signature	       db 0x29
   volumeID	       dd 'VOL_'
   volumeLabel	       db '           '
   fileSysType	       db 'FAT12   '

; -----------------------------------------------
Start:
   cli
   push  cs
   push  cs
   pop	 ds
   pop	 ss	     ; Zero Segment Regs
   mov	 sp, 0x3000
   sti

   ; 0x1000 - 0x3000  Boot Stack
   ; 0x7C00 - 0x7E00  Boot Sector
   ; 0x7E00 - 0x9000  FAT Sectors
   ; 0x9000 - 0xAC00  Root Sectors
   ; 0xAC00 - 0xF000  Second Stage Boot


   mov	 cx, 9	     ; FAT Sectors Count
   mov	 ax, 1	     ; FAT Start Sector
   mov	 bx, 0x7E0   ; FAT Start Segment

FATReadLoop:
   push  bx
   push  ax
   call  ReadSector

   inc	 ax	     ; Next Sector
   add	 bx, 0x20    ; Next Segment
   loop  FATReadLoop


   mov	 cx, 14      ; Root Sectors Count
   mov	 ax, 19      ; Root Start Sector
   mov	 bx, 0x900   ; Root Start Segment

RootReadLoop:
   push  bx
   push  ax
   call  ReadSector

   inc	 ax	     ; Next Sector
   add	 bx, 0x20    ; Next Segment
   loop  RootReadLoop


   push  Stage2FileName
   call  FindFile


   push  0xAC0	     ; Second Stage Segment
   push  ax	     ; Second Stage Cluster
   call  ReadFile


   mov	 word [0xAC00 + 2], FindFile
   mov	 word [0xAC00 + 4], ReadFile

   jmp	 0xAC00

; -----------------------------------------------
ReadFile:
   @DataSegment  = 4 + 1 * 2
   @Cluster	 = 4 + 0 * 2

   push  bp
   mov	 bp, sp
   push  ax bx dx

   mov	 dx, [bp + @DataSegment]
   mov	 ax, [bp + @Cluster]

   or	 ax, ax
   jz	 FATLoopExit  ; Zero Len File

FATLoop:
   cmp	 ax, 0x0FF8
   jae	 FATLoopExit  ; End Of Chain

   mov	 bx, ax
   add	 bx, 0x1F     ; Cluster To Sector

   push  dx	      ; Segment
   push  bx	      ; Sector
   call  ReadSector

   push  ax
   call  GetNextCluster

   add	 dx, 0x20
   jmp	 FATLoop

FATLoopExit:
   pop	 dx bx ax
   pop	 bp
   ret	 2 * 2

; -----------------------------------------------
GetNextCluster:
   @Cluster  = 4 + 0 * 2

   push  bp
   mov	 bp, sp
   push  bx cx ds

   push  0x07E0
   pop	 ds

   mov	 cx, [bp + @Cluster]
   mov	 bx, cx
   shr	 bx, 1
   add	 bx, cx
   mov	 ax, [bx]
   test  cx, 1
   jz	 Parity
   shr	 ax, 4
   jmp	 ClusterLocated

Parity:
   and	 ax, 0x0FFF

ClusterLocated:
   pop	 ds cx bx
   pop	 bp
   ret	 1 * 2

; -----------------------------------------------
FindFile:
   @FileName  = 4 + 0 * 2

   push  bp
   mov	 bp, sp
   push  cx dx si di ds es

   push  cs
   pop	 es	      ; es = 0

   push  0x0900
   pop	 ds	      ; ds = 0x0900

   xor	 si, si
   mov	 dx, 0x00E0   ; Root Entries Count

NextRootEntry:
   push  si
   mov	 cx, 0x000B   ; File Name Size

   mov	 di, [bp + @FileName]  ; File Name Buf
   repe  cmpsb
   jz	 ClusterFound

   pop	 si
   add	 si, 0x0020   ; Advance Root Entry Ptr

   dec	 dx
   jnz	 NextRootEntry
   jmp	 ReportError

ClusterFound:
   pop	 si
   mov	 ax, [si + 1Ah]
   mov	 ebx, [si + 1Ch]

   pop	 es ds di si dx cx
   pop	 bp
   ret	 1 * 2

; -----------------------------------------------
ReadSector:
   @DataSegment  = 4 + 1 * 2
   @LBA 	 = 4 + 0 * 2

   push  bp
   mov	 bp, sp
   push  ax bx cx dx si di es

   push  word [bp + @DataSegment]
   pop	 es
   mov	 ax, [bp + @LBA]

   mov	 di, 0x03    ; Retry Count

   mov	 si, 0x12    ; Sectors Per Track
   xor	 dx, dx
   div	 si

   mov	 ch, al
   shr	 ch, 1	     ; ch - Cylinder
   mov	 dh, al
   and	 dh, 1	     ; dh - Head
   mov	 cl, dl
   inc	 cl	     ; cl - Sector

   xor	 dl, dl      ; dl - Drive
   xor	 bx, bx      ; es:bx - Address

ReadRetry:
   mov	 ax, 0x0201
   int	 0x13	     ; Read Sectors

   jc	 ReadError
   jmp	 ReadOK

ReadError:
   dec	 di
   jz	 ReportError
   xor	 ah, ah
   int	 0x13	     ; Reset Disk Drives
   jmp	 ReadRetry

ReadOK:
   pop	 es di si dx cx bx ax
   pop	 bp
   ret	 2 * 2

; -----------------------------------------------
ReportError:
   mov	 al, 0x0E
   mov	 dx, 0x3D4
   out	 dx, al
   inc	 dx
   in	 al, dx
   mov	 bh, al

   mov	 al, 0x0F
   dec	 dx
   out	 dx, al
   inc	 dx
   in	 al, dx
   mov	 bl, al

   shl	 bx, 1
   push  0xB800
   pop	 ds
   mov	 word [bx], 0x0C78

   jmp	 $	     ; Hang Forever

; -----------------------------------------------
   Stage2FileName      db 'FLPBOOT BIN'

; -----------------------------------------------
ZF:
		       rb 512 - (ZF - EP) - 2
		       db 55h, 0AAh
; ===============================================