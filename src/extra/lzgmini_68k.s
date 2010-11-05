; -*- mode: asm; tab-width: 8; indent-tabs-mode: t; -*-

;-------------------------------------------------------------------------------
; This file is part of liblzg.
;
; Copyright (c) 2010 Marcus Geelnard
;
; This software is provided 'as-is',without any express or implied
; warranty. In no event will the authors be held liable for any damages
; arising from the use of this software.
;
; Permission is granted to anyone to use this software for any purpose,
; including commercial applications,and to alter it and redistribute it
; freely,subject to the following restrictions:
;
; 1. The origin of this software must not be misrepresented; you must not
;    claim that you wrote the original software. If you use this software
;    in a product,an acknowledgment in the product documentation would
;    be appreciated but is not required.
;
; 2. Altered source versions must be plainly marked as such,and must not
;    be misrepresented as being the original software.
;
; 3. This notice may not be removed or altered from any source
;    distribution.
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Description
; -----------
; This is an assembly language implemention of the LZG decoder for the Motorola
; 680x0 line of processors. It is written in Motorola syntax, and compiles with
; vasm (http://sun.hasenbraten.de/vasm/) for instance, and should be easy to
; modify for any other 68k assembler. The resulting code is about 0.5 KB, and
; requires no extra memory (except for less than 100 bytes of stack space).
;-------------------------------------------------------------------------------

;-- PRIVATE --------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------
 
LZG_HEADER_SIZE:	equ	15
LZG_METHOD_COPY:	equ	0
LZG_METHOD_LZG1:	equ	1


;-------------------------------------------------------------------------------
; LUT for decoding the copy length parameter (-1)
;-------------------------------------------------------------------------------

_LZG_LENGTH_DECODE_LUT:
	dc.b	1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
	dc.b	17,18,19,20,21,22,23,24,25,26,27,28,34,47,71,127


;-------------------------------------------------------------------------------
; _LZG_GetUINT32 - alignment independent reader for 32-bit integers
; a0 = in
; d0 = offs
; d1 = result
;-------------------------------------------------------------------------------

_LZG_GetUINT32:
	move.b	(a0,d0.l),d1
	asl.w	#8,d1
	move.b	1(a0,d0.l),d1
	swap	d1
	move.b	2(a0,d0.l),d1
	asl.w	#8,d1
	move.b	3(a0,d0.l),d1
	rts


;-------------------------------------------------------------------------------
; _LZG_CalcChecksum - Calculate the checksum
; a0 = data
; d0 = size
; d1 = result
; Clobbers: d2, d3
;-------------------------------------------------------------------------------
 
_LZG_CalcChecksum:
	movem.l	d0/a0,-(sp)
	moveq	#1,d1			; a = 1
	moveq	#0,d2			; b = 0
	moveq	#0,d3
	bra.s	.cs2
.cs1:	move.b	(a0)+,d3
	add.w	d3,d1			; a += ;data++
	add.w	d1,d2			; b += a
.cs2:	dbf	d0,.cs1
	swap	d2
	or.l	d2,d1			; return (b << 16) | a
	movem.l	(sp)+,d0/a0
	rts


;-- PUBLIC ---------------------------------------------------------------------

;-------------------------------------------------------------------------------
; LZG_Decode - Decode a compressed memory block
; a0 = in
; d0 = insize
; a1 = out
; d1 = outsize
; d2 = result (number of decompressed bytes,or zero upon failure)
;-------------------------------------------------------------------------------

LZG_Decode:

; Stack frame
inSize:		equ	0
outSize:	equ	4
encodedSize:	equ	8
decodedSize:	equ	12
checksum:	equ	16
_FRAME_SIZE:	equ	20

	movem.l	d0/d1/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4/a5,-(sp)
	lea.l	-_FRAME_SIZE(sp),sp

	; Remember caller arguments
	move.l	d0,inSize(sp)
	move.l	d1,outSize(sp)

	; Check magic ID
	cmp.l	#LZG_HEADER_SIZE,d0
	bcs	.fail
	cmp.b	#'L',(a0)
	bne	.fail
	cmp.b	#'Z',1(a0)
	bne	.fail
	cmp.b	#'G',2(a0)
	bne	.fail

	; Get header data
	moveq	#3,d0
	bsr.s	_LZG_GetUINT32
	move.l	d1,decodedSize(sp)
	moveq	#7,d0
	bsr.s	_LZG_GetUINT32
	move.l	d1,encodedSize(sp)
	moveq	#11,d0
	bsr.s	_LZG_GetUINT32
	move.l	d1,checksum(sp)

	; Check sizes
	move.l	decodedSize(sp),d7
	cmp.l	outSize(sp),d7
	bhi	.fail
	move.l	encodedSize(sp),d7
	add.l	#LZG_HEADER_SIZE,d7
	cmp.l	inSize(sp),d7
	bhi.s	.fail

	; Initialize the byte streams
	move.l	a0,a2
	add.l	inSize(sp),a2			; a2 = inEnd = in + inSize
	move.l	a1,a3
	add.l	outSize(sp),a3			; a3 = outEnd = out + outSize
	lea.l	LZG_HEADER_SIZE(a0),a0		; a0 = src
						; a1 = dst

	; Nothing to do...?
	cmp.l	a0,a2
	beq.s	.done

	; Check checksum
	move.l	encodedSize(sp),d0
	bsr	_LZG_CalcChecksum
	cmp.l	checksum(sp),d1
	bne.s	.fail

	; Check which method to use
	move.b	-1(a0),d7
	cmp.b	#LZG_METHOD_COPY,d7
	beq	.plaincopy
	cmp.b	#LZG_METHOD_LZG1,d7
	bne.s	.fail

	; Get the marker symbols
	lea.l	4(a0),a4
	cmp.l	a2,a4
	bcc.s	.fail
	move.b	(a0)+,d1			; d1 = marker1
	move.b	(a0)+,d2			; d2 = marker1
	move.b	(a0)+,d3			; d3 = marker1
	move.b	(a0)+,d4			; d4 = marker1

	; Nothing to do...?
	cmp.l	a0,a2
	beq.s	.done

	lea.l	_LZG_LENGTH_DECODE_LUT,a5	; a5 = _LZG_LENGTH_DECODE_LUT

	; Main decompression loop
	cmp.l	a2,a0
.mainloop:
	bcc.s	.fail				; Note: cmp.l a2,a0 must be performed prior to this!
	move.b	(a0)+,d7			; d7 = symbol

	cmp.b	d1,d7				; marker1?
	beq.s	.marker1
	cmp.b	d2,d7				; marker2?
	beq.s	.marker2
	cmp.b	d3,d7				; marker3?
	beq.s	.marker3
	cmp.b	d4,d7				; marker4?
	beq.s	.marker4

.literal:
	cmp.l	a3,a1
	bcc.s	.fail
	move.b	d7,(a1)+
	cmp.l	a2,a0
	bcs.s	.mainloop

	; We're done
.done:
	cmp.l	a3,a1
	bne.s	.fail
	move.l	decodedSize(sp),d2
	bra.s	.exit

	; This is where we end up if something went wrong...
.fail:
	moveq	#0,d2
.exit:	lea.l	_FRAME_SIZE(sp),sp
	movem.l	(sp)+,d0/d1/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4/a5
	rts

	; marker4 - "Near copy (incl. RLE)"
.marker4:
	cmp.l	a2,a0
	bcc.s	.fail
	moveq	#0,d5
	move.b	(a0)+,d5
	beq.s	.literal			; Single occurance of the marker symbol (rare)
	move.w	d5,d6
	and.b	#$1f,d6
	move.b	(a5,d6.w),d6			; length-1 = _LZG_LENGTH_DECODE_LUT[b & 0x1f]
	lsr.b	#5,d5
	addq.w	#1,d5				; offset = (b >> 5) + 1
	bra.s	.copy

	; marker3 - "Short copy"
.marker3:
	cmp.l	a2,a0
	bcc.s	.fail
	moveq	#0,d5
	move.b	(a0)+,d5
	beq.s	.literal			; Single occurance of the marker symbol (rare)
	move.w	d5,d6
	asr.b	#6,d6
	addq.w	#2,d6				; length-1 = (b >> 6) + 2
	and.b	#$3f,d5
	addq.w	#8,d5				; offset = (b & 0x3f) + 8
	bra.s	.copy

	; marker2 - "Medium copy"
.marker2:
	cmp.l	a2,a0
	bcc.s	.fail
	moveq	#0,d5
	move.b	(a0)+,d5
	beq.s	.literal			; Single occurance of the marker symbol (rare)
	cmp.l	a2,a0
	bcc.s	.fail
	move.l	d5,d6
	and.b	#$1f,d6
	move.b	(a5,d6.w),d6			; length-1 = _LZG_LENGTH_DECODE_LUT[b & 0x1f]
	lsl.w	#3,d5
	move.b	(a0)+,d5
	addq.w	#8,d5				; offset = ((b & 0xe0) << 3) | b2
	bra.s	.copy

	; marker1 - "Distant copy"
.marker1:
	cmp.l	a2,a0
	bcc.s	.fail
	moveq	#0,d5
	move.b	(a0)+,d5
	beq.s	.literal			; Single occurance of the marker symbol (rare)
	lea.l	2(a0),a4
	cmp.l	a2,a4
	bcc.s	.fail
	move.l	d5,d6
	and.b	#$1f,d6
	move.b	(a5,d6.w),d6			; length-1 = _LZG_LENGTH_DECODE_LUT[b & 0x1f]
	lsr.w	#5,d5
	swap	d5
	move.b	(a0)+,d5
	lsl.w	#8,d5
	move.b	(a0)+,d5
	add.w	#2056,d5			; offset = ((b & 0xe0) << 11) | (b2 << 8) | (*src++)

	; Copy corresponding data from history window
	; d5 = offset
	; d6 = length-1
.copy:
	move.l	a1,a4
	sub.l	d5,a4
	; FIXME: if (!((copy >= out) && ((dst + length) <= outEnd))) return 0;
.loop1:	move.b	(a4)+,(a1)+
	dbf	d6,.loop1

	cmp.l	a2,a0
	bcs	.mainloop

	; For non-compressible data, we copy the data as it is (1:1)
.plaincopy:
	move.l	encodedSize(sp),d6
	cmp.l	decodedSize(sp),d6
	bne	.fail
	subq.l	#1,d6
.loop2:	move.b	(a0)+,(a1)+
	dbf	d6,.loop2
	bra	.done

