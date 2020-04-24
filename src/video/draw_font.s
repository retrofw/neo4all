! -----------------------------------------------------------
! draw_font function for Neo4all emulator by Chui
! Oscar Orallo Pelaez
! Date: 04/10/2005
!
! Parameters:
! unsigned short *br, unsigned short *paldata, unsigned *gfxdata
! r4 = br
! r5 = paldata
! r6 = gfxdata
! -----------------------------------------------------------
.text
.balign 32
.global _draw_font
_draw_font:
! Set store queue
	pref @r6
	mov r4,r2               ! salvaguardar destino
	mov.l sq_andmask,r0
	and r0,r4
	mov.l sq_ormask,r0
	or r0,r4		! (0xe0000000 | (((unsigned long)dest) & 0x03ffffe0))

.ifdef AES
        mov #-24,r3
        mov r2,r0
        shld r3,r0
        mov #21,r3
        and #0x1C,r0
        shld r3,r0
        mov #16,r1        ! r1 = loop counter
        or r0,r4
.else
	shlr16 r2
	mov #0x1c,r1
	shlr8 r2
	mov.l QACR0,r3
	and r1,r2		! QACR0 = ((((unsigned int)dest)>>26)<<2)&0x1c
	
	mov.l r2,@r3
	mov.l r2,@(4,r3)
.endif

	mov #4,r3         ! r3 = outer loop counter (y)
	add #16,r4        ! getting up br to poke

mloop:
	mov.l @r6+,r1     ! r1 = mydword
	mov #-32,r2       ! r2 = shifting value
	mov r1,r0

iloop:
	add #4,r2
	shld r2,r0
	and #0x0F,r0      ! r0 = paldata index

! Poking to br...
	shll r0           ! r0<<=1
	mov.w @(r0,r5),r7 ! r7 = paldata[index]
	
	add #4,r2
	mov r1,r0
	shld r2,r0
	and #0x0F,r0
	shll r0
	tst r2,r2         ! testing shifting value
	mov.w @(r0,r5),r0
	shll16 r0
	xtrct r7,r0
	mov.l r0,@-r4

	bf/s iloop
	mov r1,r0

	add #32,r4

	mov.l @r6+,r1
	mov #-32,r2
	mov r1,r0

iloop2:
	add #4,r2
	shld r2,r0
	and #0x0F,r0	! r0 = paldata index

! Poking to br...
	shll r0           ! r0<<=1
	mov.w @(r0,r5),r7 ! r7 = paldata[index]
	
	add #4,r2
	mov r1,r0
	shld r2,r0
	and #0x0F,r0
	shll r0
	tst r2,r2         ! testing shifting value
	mov.w @(r0,r5),r0
	shll16 r0
	xtrct r7,r0
	mov.l r0,@-r4

	bf/s iloop2
	mov r1,r0

	add #-16,r4
	pref @r4

	dt r3
	bf/s mloop
	add #48,r4        ! updating br

! FINALIZA SQ 
	mov #0,r0
	mov.l sq_ormask,r1
	mov.l r0,@(32,r1)
	rts
	mov.l r0,@r1
! nop

.align 2
sq_andmask: 	.long 0x03ffffe0
sq_ormask:  	.long 0xe0000000
QACR0:  	.long 0xff000038

