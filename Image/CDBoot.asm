; ===============================================
org 0x7C00
use16

; -----------------------------------------------
EP:
   cli
   push  0
   pop	 ds
   mov	 [DriveIndex], dl
   push  ds
   pop	 ss	     ; Zero Segment Regs
   mov	 sp, 0x3000
   sti

   ; 0x1000 - 0x3000  Boot Stack
   ; 0x7C00 - 0x8400  Boot Sector
   ; 0x8400 - 0x8C00  Primary VD
   ; 0x8C00 - 0xF000  Root Dir

   push  0x840
   pushd 16
   call  ReadSector

   mov	 eax, [0x8400 + 0x9E]  ; Sector
   mov	 ebx, [0x8400 + 0xA6]  ; Size
   mov	 [RootSize], ebx

   pushd ebx
   push  0x8c0
   pushd eax
   call  ReadFile


   mov	 dx, 0x2000  ; Load Segment
   mov	 si, File1Size

NextFile:
   push  word [si + 8]
   call  FindFile

   mov	 [si + 0], ebx
   mov	 ecx, ebx
   add	 ecx, 0xFFF
   shr	 ecx, 12     ; Pages Count

   pushd ebx
   push  dx
   pushd eax
   call  ReadFile

   shr	 dx, 8
   mov	 [si + 4], dx
   add	 dx, cx
   shl	 dx, 8

   add	 si, 12
   cmp	 si, File1Name
   jnz	 NextFile


   cli


   ; Enable A20
   call  KeybWait
   mov	 al, 0xD1
   out	 0x64, al
   call  KeybWait
   mov	 al, 0xDF
   out	 0x60, al


   mov	 dx, 0x03F2
   xor	 al, al
   out	 dx, al       ; Reset Floppy + Stop Motor


   push  0
   pop	 ds

   lgdt  fword ptr ds:GDT_PDescr

   mov	 eax, cr0
   or	 al, 1
   mov	 cr0, eax

   mov	 ax, 0x08
   mov	 ds, ax
   mov	 es, ax
   mov	 fs, ax
   mov	 gs, ax
   mov	 ss, ax
   xor	 ebp, ebp

   mov	 dword [0x20100], Magic

   jmp	 far 0x10:Prot

; -------------------------------------------
   use32

; -------------------------------------------
Prot:
   jmp	 dword [0x20008]

; -------------------------------------------
GDT_PDescr  dw 0x0FFF
	    dd GDT

GDT:  db 8 dup(0)
      db 2 dup(0FFh), 3 dup(0), 92h, 0CFh, 0 ; GDT Data Descriptor
      db 2 dup(0FFh), 3 dup(0), 98h, 0CFh, 0 ; GDT Code Descriptor

; -------------------------------------------
   use16

; -------------------------------------------
KeybWait:
   push  ax

WaitLoop:
   in	 al, 0x64
   and	 al, 2
   jnz	 WaitLoop

   pop	 ax
   ret

; -----------------------------------------------
FindFile:
   @FileName  = 4 + 0 * 2

   push  bp
   mov	 bp, sp
   push  cx dx si di ds es

   push  0
   pop	 es	      ; es = 0
   push  es
   pop	 ds	      ; ds = 0

   mov	 bx, 0x8c00
   mov	 dx, bx
   add	 dx, word [RootSize]
   mov	 si, bx
   add	 si, 0x21
   xor	 cx, cx
   xor	 ax, ax

NextRootEntry:
   cmp	 bx, dx
   jae	 ReportError

   mov	 al, [bx]
   mov	 cl, [bx + 0x20]

   cmp	 al, 0
   jnz	 @f
   inc	 bx
   inc	 si
   jmp	 NextRootEntry
@@:

   push  si
   mov	 di, [bp + @FileName]  ; File Name Buf
   repe  cmpsb
   jz	 SectorFound

   pop	 si
   add	 bx, ax
   add	 si, ax

   jmp	 NextRootEntry

SectorFound:
   pop	 si
   mov	 eax, [bx + 0x2]
   mov	 ebx, [bx + 0xA]

   pop	 es ds di si dx cx
   pop	 bp
   ret	 1 * 2

; -----------------------------------------------
ReadFile:
   @FileSize	 = 4 + 3 * 2
   @DataSegment  = 4 + 2 * 2
   @LBA 	 = 4 + 0 * 2

   push  bp
   mov	 bp, sp
   push  ax cx dx

   mov	 ecx, [bp + @FileSize]
   add	 ecx, 0x7FF
   shr	 ecx, 11

   mov	 dx, [bp + @DataSegment]
   mov	 eax, [bp + @LBA]

NextSector:
   push  dx
   pushd eax
   call  ReadSector

   inc	 eax
   add	 dx, 0x0080
   loop  NextSector

   pop	 dx cx ax
   pop	 bp
   ret	 2 * 5

; -----------------------------------------------
ReadSector:
   @DataSegment  = 4 + 2 * 2
   @LBA 	 = 4 + 0 * 2

   push  bp
   mov	 bp, sp
   push  ax dx si

   mov	 ax, [bp + @DataSegment]
   mov	 [PacketDataSegment], ax

   mov	 eax, [bp + @LBA]
   mov	 [PacketLBALow], eax

   mov	 ah, 0x42
   mov	 dl, [DriveIndex]
   mov	 si, DiskAddressPacket
   int	 0x13	     ; Extended Read Sectors

   jc	 ReportError

   pop	 si dx ax
   pop	 bp
   ret	 2 * 3

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
align 4
DiskAddressPacket:
		       db 0x10	  ; PacketSize
		       db 0x00
		       dw 0x0001  ; BlockCount
		       dw 0x0000  ; DataOffset
   PacketDataSegment   dw 0x0000
   PacketLBALow        dd 0x00000000
		       dd 0x00000000

; -----------------------------------------------
   DriveIndex	       db 0x00
   RootSize	       dd 0x00000000

; -----------------------------------------------
   Magic	       dd 'linf'
   FilesCount	       dd 6

   File1Size	       dd 0
   File1LoadPage       dd 0
   File1NamePtr        dd File1Name
   File2Size	       dd 0
   File2LoadPage       dd 0
   File2NamePtr        dd File2Name
   File3Size	       dd 0
   File3LoadPage       dd 0
   File3NamePtr        dd File3Name
   File4Size	       dd 0
   File4LoadPage       dd 0
   File4NamePtr        dd File4Name
   File5Size	       dd 0
   File5LoadPage       dd 0
   File5NamePtr        dd File5Name
   File6Size	       dd 0
   File6LoadPage       dd 0
   File6NamePtr        dd File6Name

; -----------------------------------------------
   File1Name	       db 'Kernel.bin', 0
   File2Name	       db 'FileSys.bin', 0
   File3Name	       db 'CDFS.bin', 0
   File4Name	       db 'Cache.bin', 0
   File5Name	       db 'ATA.bin', 0
   File6Name	       db 'Loader.bin', 0

; -----------------------------------------------
ZF:
		       rb 2048 - (ZF - EP) - 2
		       db 55h, 0AAh
; ===============================================