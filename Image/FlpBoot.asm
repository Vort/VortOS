; ===========================================
org 0xAC00
use16

; -------------------------------------------
EP:
   jmp	 Start

; -------------------------------------------
   FindFileFunc   dw 0x0000
   ReadFileFunc   dw 0x0000

; -------------------------------------------
Start:
   cmp	 byte [0x7C15], 0xF0
   je	 @f
     mov byte [BootType + 2], '5'
@@:

   mov	 dx, 0x2000  ; Load Segment
   mov	 di, File1FATName
   mov	 si, File1Size

NextFile:
   push  di
   call  [FindFileFunc]

   mov	 [si + 0], ebx
   mov	 ecx, ebx
   add	 ecx, 0xFFF
   shr	 ecx, 12     ; Pages Count

   push  dx
   push  ax
   call  [ReadFileFunc]

   shr	 dx, 8
   mov	 [si + 4], dx
   add	 dx, cx
   shl	 dx, 8

   add	 si, 12
   add	 di, 11
   cmp	 di, LastFATName
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


   push  cs
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

; -------------------------------------------
   Magic	      dd 'linf'
   BootType	      dd 'fd3 '
   FilesCount	      dd 7

   File1Size	      dd 0
   File1LoadPage      dd 0
   File1NamePtr       dd File1Name
   File2Size	      dd 0
   File2LoadPage      dd 0
   File2NamePtr       dd File2Name
   File3Size	      dd 0
   File3LoadPage      dd 0
   File3NamePtr       dd File3Name
   File4Size	      dd 0
   File4LoadPage      dd 0
   File4NamePtr       dd File4Name
   File5Size	      dd 0
   File5LoadPage      dd 0
   File5NamePtr       dd File5Name
   File6Size	      dd 0
   File6LoadPage      dd 0
   File6NamePtr       dd File6Name
   File7Size	      dd 0
   File7LoadPage      dd 0
   File7NamePtr       dd File7Name

; -------------------------------------------
   File1Name	      db 'Kernel.bin', 0
   File2Name	      db 'FileSys.bin', 0
   File3Name	      db 'FAT.bin', 0
   File4Name	      db 'Cache.bin', 0
   File5Name	      db 'DMA.bin', 0
   File6Name	      db 'Floppy.bin', 0
   File7Name	      db 'SysInit.bin', 0

; -------------------------------------------
   File1FATName       db 'KERNEL  BIN'
   File2FATName       db 'FILESYS BIN'
   File3FATName       db 'FAT     BIN'
   File4FATName       db 'CACHE   BIN'
   File5FATName       db 'ISADMA  BIN'
   File6FATName       db 'FLOPPY  BIN'
   File7FATName       db 'SYSINIT BIN'
   LastFATName:
; ===========================================