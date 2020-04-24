! -----------------------------------------------------------
! Draw tile function
! Oscar Orallo Pelaez
! Created on: 04/06/2005
! Last modified: 04/12/2005
! Parameters:
! br     (word ptr)   r4
! paldata (word ptr)  r5
! gfxdata (dword ptr)  r6
! -----------------------------------------------------------
.text
.balign 32
.global _draw_tile
_draw_tile:

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
	mov #16,r1        ! r1 = loop counter
	mov.l r2,@(4,r3)
.endif


mloop:
	
	add #16,r4        ! r4 = br + displacement
.ifdef AES
	mov.l @(4,r6),r3
.else
	mov.l @r6+,r3     ! r3 = mydword
.endif
	mov #-28,r7       ! r7 = shifting value
	
.iloop:
	mov r3,r0
	shld r7,r0
	and #0x0F,r0
	shll r0
	add #4,r7
	mov.w @(r0,r5),r2
	mov r3,r0
	shld r7,r0
	and #0x0F,r0
	shll r0
	mov.w @(r0,r5),r0
	extu.w r2,r2
	shll16 r0
	or r2,r0
	
	
	cmp/pz r7
	mov.l r0,@r4        ! Escritura destino efectiva
	add #4,r7
	bf/s .iloop
	add #4,r4

.ifdef AES
	mov.l @r6,r3
	add #8,r6
.else
	mov.l @r6+,r3
.endif

	add #-32,r4
	mov #-28,r7       ! r7 = shifting value

.sloop:
	mov r3,r0
	shld r7,r0
	and #0x0F,r0
	shll r0
	mov.w @(r0,r5),r2
	add #4,r7
	mov r3,r0
	shld r7,r0
	and #0x0F,r0
	shll r0
	mov.w @(r0,r5),r0
	extu.w r2,r2
	shll16 r0
	or r2,r0

	cmp/pz r7
	mov.l r0,@r4
	add #4,r7
	bf/s .sloop
	add #4,r4

	add #-16,r4
	pref @r4

	dt r1
	bf/s mloop
	add #32,r4


! FINALIZA SQ 
	mov #0,r0
	mov.l sq_ormask,r1
	mov.l r0,@(32,r1)
	rts
	mov.l r0,@r1

.align 2
sq_andmask: 	.long 0x03ffffe0
sq_ormask:  	.long 0xe0000000
.ifndef AES
	QACR0:  	.long 0xff000038
.endif
