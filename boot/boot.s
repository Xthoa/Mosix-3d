org 0x7c00

%macro Descriptor 3
	dw %2 & 0FFFFh
	dw %1 & 0FFFFh
	db ( %1 >> 16) & 0FFh
	dw (( %2 >> 8) & 0F00h) | ( %3 & 0F0FFh)
	db ( %1 >> 24) & 0FFh
%endmacro

SEG_RPL equ 1
DPL equ 0x20
SEG64 equ 0x2000
SEG32 equ 0x4000
SEG_PG4K equ 0x8000
SEG_P equ 0x80
SEG_SEG equ 0x10
SEG_AC equ 0x1	;accessed
SEG_RW equ 0x2
SEG_DR equ 0x4	;direction
SEG_EX equ 0x8

SEG_BASE equ SEG_P|SEG_SEG
SEG_DATA equ SEG_BASE|SEG_RW
SEG_CODE equ SEG_BASE|SEG_RW|SEG_EX
SEG_DATA32 equ SEG_DATA|SEG32|SEG_PG4K
SEG_CODE32 equ SEG_CODE|SEG32|SEG_PG4K
SEG_DATA64 equ SEG_DATA|SEG64
SEG_CODE64 equ SEG_CODE|SEG64

PML4_64_PHY equ 0x1000
PDPT_64_PHY equ 0x2000
PDPTS_64_PHY equ 0x54000
PD0_64_PHY equ 0x3000
PT0_64_PHY equ 0x4000
PT1_64_PHY equ 0x5000
PT2_64_PHY equ 0x6000
PDS_64_PHY equ 0x50000
PTS_64_PHY equ 0x51000

bits 16

jmp entry
nop

oemname db "MOSIX 3D"
sectorsz dw 512
clustsz db 1
rsvdsecs dw 2
fats db 2
dentries dw 224
sectors dw 2880
mediatype db 0xf0
fatsz dw 9
tracksz dw 18
heads dw 2
hiddensz dd 0
sectors32 dd 0
drivenof db 0
ntflag db 0
sign db 0x29
serial dd 0
volname db "Mosix Dummy"
sysid db "FAT12   "

entry:
jmp 0:entry16
entry16:

cli
cld

mov ax,0
mov ds,ax
mov ss,ax
mov sp,0x7c00
mov [driveno], dl

KernelSectors equ 70
ArchiveSectors equ 16
ArchivePosition equ 76

mov cx,0x7e0
mov di,1
mov si,3
call read_media
mov cx,0x1000
mov di,4
mov si,KernelSectors
call read_media
mov cx,0x3000
mov di,ArchivePosition
mov si,ArchiveSectors
call read_media
mov ax,0
mov [es:0x200],ax
mov es,ax
jmp afterread

read_media:
; di = lba
; si = size
; cx = pos>>4
    push bp
    push bx
    mov bp,si
    mov es,cx
    call lba2chs_fdd
    readloop:
    mov ah,2
    mov al,1
    mov bx,0
    mov dl,[driveno]
    int 13h
    jc err
    dec bp
    jz readend
    mov ax,es
    add ax,0x20
    mov es,ax
    add cl,1
    cmp cl,18
    jbe readloop
    mov cl,1
    inc dh
    cmp dh,2
    jb readloop
    mov dh,0
    inc ch
    jmp readloop
    readend:
    pop bx
    pop bp
    ret

lba2chs_fdd:
; di = lba
; +cl = sector
; +dh = head
; +ch = cylinder
    mov ax,di
    mov cl,18
    div cl
    mov ch,al
    shr ch,1
    mov dh,al
    and dh,1
    mov cl,ah
    inc cl
    ret


Bootseg equ 0x5800

afterread:

; call set_video_mode16
call probe_memory16

mov ax,0
mov ds,ax
mov es,ax

lgdt [GDTR32]

in al,92h
or al,2
out 92h,al

mov eax,cr0
or al,1		;cr0.PE
mov cr0,eax
jmp dword 48: entry32

bits 32
entry32:
mov ax,40
mov ds,ax
mov es,ax
mov fs,ax
mov gs,ax
mov ss,ax
mov esp,0x7c00

mov eax,0
mov edi,PML4_64_PHY
mov ecx,0x6000/4
rep stosd
mov edi,0xc000
mov ecx,0x1000/4
rep stosd
mov esi,GDT
mov edi,0xa000
mov ecx,9*8/4
rep movsd
jmp next32

bits 16
probe_memory16:
; modified: si, di, bx, cx, dx
    mov ax,Bootseg
    mov es,ax
    mov si,0
    mov di,0x2400
    xor bx,bx
    .loop:
    mov eax,0xE820
    mov edx,0x534D4150
    mov ecx,20
    int 15h
    add di,20
    inc si
    or ebx,ebx
    jnz .loop
    mov [es:0x2010],si
    mov word [es:0x2012],0x2400
    ret

err:
hlt
jmp err

driveno db 0
times 446-($-$$) db 0
times 64 db 0
db 0x55,0xaa

apmain dq 0

bits 16
set_video_mode16:
; modified: esi, di, bx, cx, dx
;  +c u32 lfb phy
    ; VBEMODE equ 0x117	; 1024*768*16bpp
    VBEMODE equ 0x114	; 800*600*16bpp
    mov ax,Bootseg
    mov es,ax
; Temporarily, we use VBEMODE as the default mode
; If don't want default mode, comment the line below
    jmp defmode
; Auto select mode
    CheckBpp equ 32
    CheckXsize equ 1024
    CheckYsize equ 768	; check 1024*768*32bpp
; 1. Get all display modes
	mov di,0
	mov ax,0x4f00	; 00h - Return VGA info
	int 0x10
	cmp dword [es:di],0x41534556	; Check 'VESA'
	jnz err
	mov dx,[es:di+16]	; Video modes ptr
	mov ds,dx
	shl edx,4
	mov bx,[es:di+14]
	add dx,bx
	mov [es:0x2000],edx     ; store it
; 2. Get all display modes' info
	mov di,0x200
	.lp:
		mov cx,[bx]
		cmp cx,0xffff   ; end
		jz .end
		mov ax,0x4f01	; 01h - Return mode info
		int 0x10
		; Check if we need it
			cmp word [es:0x2008],0
			jnz .n2
			mov dx,[es:di]	; ModeAttributes
			; Graphics, colored, supported
			and dx,00011001b
			cmp dx,00011001b
			jnz .n2
			mov dl,[es:di+25]	; BitsPerPixel
			cmp dl,CheckBpp	; need 32bpp
			jnz .n2
			mov dx,[es:di+18]	; X size
			cmp dx,CheckXsize
			jnz .n2
			mov [es:0x2010],dx
			mov dx,[es:di+20]	; Y size
			cmp dx,CheckYsize
			jnz .n2
			mov [es:0x2012],dx
			mov [es:0x2008],cx
			mov [es:0x200a],di
			mov esi,[es:di+40]	; Linear buffer
			mov [es:0x200c],esi
		.n2:
		add di,0x100
		add bx,2
		jmp .lp
	.end:
    mov ax,0
    mov ds,ax
	jmp setmode
; 3. Set the display mode
defmode:
	mov cx,VBEMODE
	mov di,0x200
	mov [es:0x2008],cx
	mov [es:0x200a],di
	mov ax,0x4f01
	int 0x10
	mov dx,[es:di+18]	; X size
	mov [es:0x2004],dx
	mov dx,[es:di+20]	; Y size
	mov [es:0x2006],dx
	mov esi,[es:di+40]	; Linear buffer
	mov [es:0x200c],esi
setmode:
	mov ax,0x4f02
	mov bx,[es:0x2008]
	add bx,0x4000
	int 0x10
	mov ax,0
	mov es,ax
    ret

bits 32
next32:
mov eax,PML4_64_PHY
mov cr3,eax

mov dword [eax+0xff0],0x3+PML4_64_PHY   ; trick on self-refing
mov dword [eax+0xff8],0x3+PDPT_64_PHY
mov ebx,PDPT_64_PHY
mov dword [ebx],0x3+PD0_64_PHY
mov ebx,PD0_64_PHY
mov dword [ebx],0x3+PT0_64_PHY
mov dword [ebx+8],0x3+PT1_64_PHY

mov dword [eax],0x3+PDPTS_64_PHY
mov ebx,PDPTS_64_PHY
mov dword [ebx],0x3+PDS_64_PHY
mov ebx,PDS_64_PHY
mov dword [ebx],0x3+PTS_64_PHY
mov esi,PTS_64_PHY
mov dword [esi+7*8],0x7003
mov dword [esi+8*8],0x8003

mov edi,PT0_64_PHY
mov eax,0x10003
mov ecx,64
call fill_pt32
mov eax,0xc003
mov ecx,4
call fill_pt32
mov eax,0x54003
mov ecx,12
call fill_pt32
mov eax,0x1003
stosd
add edi,4
mov eax,0x9003
mov ecx,2
call fill_pt32
mov eax,0x60003
stosd
add edi,44
mov eax,0x61003
mov ecx,9
call fill_pt32
mov eax,0x70003
mov ecx,4
call fill_pt32
mov eax,0xb8003
stosd
mov edi,PT1_64_PHY
mov eax,0x80003
mov ecx,0x1c
call fill_pt32

call find_rsdp32
jmp next64

times 1024-($-$$) nop
; 0x8000
bits 16
jmp 0:apentry
bits 32

fill_pt32:
    stosd
    add edi,4
    add eax,0x1000
    loop fill_pt32
    ret

find_rsdp32:
    mov ebp,0x5a020
	mov dword [ebp+4],0
    xor esi,esi
    mov si,word [0x40e]
    shl esi,4
    mov ecx,1024
    call find_rsdp_tag32
    jnc .found1
    mov esi,0xe0000
    mov ecx,0x20000
    call find_rsdp_tag32
    .found1:
    mov [ebp],eax
    ret

find_rsdp_tag32:
    mov ebx,"RSD "
    mov edx,"PTR "
    shr ecx,4
    .lp:
    lodsb
    cmp al,bl
    jz .lpa
    add esi,15
    loop .lp
    jmp .none
    .lpa:
    dec esi
    lodsd
    cmp eax,ebx
    jz .lpb
    add esi,12
    loop .lp
    jmp .none
    .lpb:
    lodsd
    cmp eax,edx
    jz .found
    add esi,8
    loop .lp
    .none:
    xor eax,eax
    stc
    ret
    .found:
    lea eax,[esi-8]
    clc
    ret

next64:
mov eax,cr4
or eax,0x2b0
mov cr4,eax
mov ecx,0xc0000080
rdmsr
or eax,0x901
wrmsr
mov eax,cr0
bts eax,31
mov cr0,eax
jmp 8: entry64

bits 64
entry64:
lgdt [GDTR64]
lidt [IDTR64]
mov ax,16
mov ds,ax
mov es,ax
mov ss,ax
mov fs,ax
mov gs,ax

mov rbx, 0xffffff80_00000000
lea rsi, [rbx+0x48000]
mov rsp, rsi

lea rdi, [rsi+0x2000]
movzx rax, word [rdi+0xa]
add rax, rsi
mov [rdi+0x28], rax
movzx rax, word [rdi+0x12]
add rax, rsi
mov [rdi+0x30], rax

cli
add rbx, 0x1000
call rbx

cli
end:
hlt
jmp end

apentry:
    bits 16
    cli
	wbinvd
	mov ax,0
	mov ds,ax
	lgdt [GDTR32]
	mov eax,cr0
	or eax,1
	mov cr0,eax
	jmp dword 48:apcode32
apcode32:
	bits 32
	mov ax,40
	mov ds,ax
	mov eax,0x1000
	mov cr3,eax
	mov eax,cr4
	or eax,0x2b0
	mov cr4,eax
	mov ecx,0xc0000080
	rdmsr
	or eax,0x901
	wrmsr
	mov eax,cr0
	bts eax,31
	mov cr0,eax
	jmp 8:apcode64
apcode64:
	bits 64
	lgdt [rel GDTR64]
	lidt [rel IDTR64]
	mov ax,16
	mov ds,ax
	mov es,ax
	mov ss,ax
	mov fs,ax
	mov gs,ax

jmp [apmain]

align 16
GDT:
	Descriptor 0,0,0
	Descriptor 0,0,SEG_CODE64
	Descriptor 0,0,SEG_DATA64
	Descriptor 0,0,SEG_DATA64+DPL*3
	Descriptor 0,0,SEG_CODE64+DPL*3
	Descriptor 0,0xfffff,SEG_DATA32
	Descriptor 0,0xfffff,SEG_CODE32
	Descriptor 0,0xfffff,SEG_DATA32+DPL*3
	Descriptor 0,0xfffff,SEG_CODE32+DPL*3
GDTR32:
	dw 0xff
	dd GDT
GDTR64:
	dw 0xfff
	dq 0xffffff8000052000
IDTR64:
	dw 0xfff
	dq 0xffffff8000051000

times 2048-($-$$) db 0

; boot arguments structure
; +0 VESA info
; +0x200 video modes info
; +0x2000 main table
; +0x2400 e820 memory map

; main table (16bit part)
; +0 video modes map (phy)
; +4 xsize
; +6 ysize
; +8 mode
; +a mode info off (from bootseg)
; +c lfb
; +10 ards block count
; +12 ards off (from bootseg)
; +14 
; +18 smbios phy64 (unsupported)
; +20 rsdp/xsdp phy64
; +28 mode info pos (fixedup)
; +30 ards pos (fixedup)