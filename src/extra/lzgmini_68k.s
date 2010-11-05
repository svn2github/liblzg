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

;-- PRIVATE --------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------
 
LZG_HEADER_SIZE:	equ	15
LZG_METHOD_COPY:	equ	0
LZG_METHOD_LZG1:	equ	1


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
;-------------------------------------------------------------------------------
 
_LZG_CalcChecksum:
	movem.l	d0/d2/d3/a0,-(sp)
	moveq	#1,d1			; a = 1
	moveq	#0,d2			; b = 0
	moveq	#0,d3
	bra.s	cs2
cs1:	move.b	(a0)+,d3
	add.w	d3,d1			; a += ;data++
	add.w	d1,d2			; b += a
cs2:	dbf	d0,cs1
	swap	d2
	or.l	d2,d1			; return (b << 16) | a
	movem.l	(sp)+,d0/d2/d3/a0
	rts


;-------------------------------------------------------------------------------
; LUT for decoding the copy length parameter
;-------------------------------------------------------------------------------

_LZG_LENGTH_DECODE_LUT:
	dc.b	2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17
	dc.b	18,19,20,21,22,23,24,25,26,27,28,29,35,48,72,128



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
STACK_FRAME_SIZE: equ	20

	; Macro for checking against in data array bounds (clobbers a4)
	macro	BOUNDS_CHECK_IN
	ifeq	\1-1
	cmp.l	a2,a0
	else
	lea.l	\1(a0),a4
	cmp.l	a2,a4
	endif
	bcc	_fail
	endm

	; Macro for checking against out data array bounds (clobbers a4)
	macro	BOUNDS_CHECK_OUT
	ifeq	\1-1
	cmp.l	a3,a1
	else
	lea.l	\1(a1),a4
	cmp.l	a3,a4
	endif
	bcc	_fail
	endm

	movem.l	d0/d1/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4/a5,-(sp)
	lea.l	-STACK_FRAME_SIZE(sp),sp

	; Remember caller arguments
	move.l	d0,inSize(sp)
	move.l	d1,outSize(sp)

	; Check magic ID
	cmp.l	#LZG_HEADER_SIZE,d0
	bcs	_fail
	cmp.b	#'L',(a0)
	bne	_fail
	cmp.b	#'Z',1(a0)
	bne	_fail
	cmp.b	#'G',2(a0)
	bne	_fail

	; Get header data
	moveq	#3,d0
	bsr	_LZG_GetUINT32
	move.l	d1,decodedSize(sp)
	moveq	#7,d0
	bsr	_LZG_GetUINT32
	move.l	d1,encodedSize(sp)
	moveq	#11,d0
	bsr	_LZG_GetUINT32
	move.l	d1,checksum(sp)

	; Check sizes
	move.l	decodedSize(sp),d2
	cmp.l	outSize(sp),d2
	bhi	_fail
	move.l	encodedSize(sp),d2
	add.l	#LZG_HEADER_SIZE,d2
	cmp.l	inSize(sp),d2
	bhi	_fail

	; Initialize the byte streams
	move.l	a0,d2
	add.l	inSize(sp),d2
	move.l	d2,a2				; a2 = inEnd = in + inSize
	move.l	a1,d2
	add.l	outSize(sp),d2
	move.l	d2,a3				; a3 = outEnd = out + outSize
	lea.l	LZG_HEADER_SIZE(a0),a0		; a0 = src
						; a1 = dst
	lea.l	_LZG_LENGTH_DECODE_LUT,a5	; a5 = _LZG_LENGTH_DECODE_LUT

	; Nothing to do...?
	cmp.l	a0,a2
	beq	_done

	; Check checksum
	move.l	encodedSize(sp),d0
	bsr	_LZG_CalcChecksum
	cmp.l	checksum(sp),d1
	bne	_fail

	; Check which method to use
	move.b	-1(a0),d2
	cmp.b	#LZG_METHOD_COPY,d2
	beq	_plainCopy
	cmp.b	#LZG_METHOD_LZG1,d2
	bne	_fail

	; Get the marker symbols
	BOUNDS_CHECK_IN 4
	move.b	(a0)+,d1			; d1 = marker1
	move.b	(a0)+,d2			; d2 = marker1
	move.b	(a0)+,d3			; d3 = marker1
	move.b	(a0)+,d4			; d4 = marker1

	; Nothing to do...?
	cmp.l	a0,a2
	beq	_done

dc1:	; Main decompression loop
	BOUNDS_CHECK_IN 1
	move.b	(a0)+,d0			; d0 = symbol

	cmp.b	d1,d0				; marker1?
	beq.s	dc3
	cmp.b	d2,d0				; marker2?
	beq.s	dc4
	cmp.b	d3,d0				; marker3?
	beq.s	dc5
	cmp.b	d4,d0				; marker4?
	beq.s	dc6

dc2:	BOUNDS_CHECK_OUT 1
	move.b	d0,(a1)+
	cmp.l	a2,a0
	bcs.s	dc1
	bra	_done

dc3:	; marker1 - "Distant copy"
	BOUNDS_CHECK_IN 1
	moveq	#0,d5
	move.b	(a0)+,d5
	beq	dc2				; Single occurance of the marker symbol (rare)
	BOUNDS_CHECK_IN 2
	move.l	d5,d6
	and.b	#$1f,d6
	move.b	(a5,d6.w),d6			; length = _LZG_LENGTH_DECODE_LUT[b & 0x1f]
	lsr.w	#5,d5
	swap	d5
	move.b	(a0)+,d5
	lsl.w	#8,d5
	move.b	(a0)+,d5
	add.w	#2056,d5			; offset = ((b & 0xe0) << 11) | (b2 << 8) | (*src++)
	bra.s	dc7

dc4:	; marker2 - "Medium copy"
	BOUNDS_CHECK_IN 1
	moveq	#0,d5
	move.b	(a0)+,d5
	beq	dc2				; Single occurance of the marker symbol (rare)
	BOUNDS_CHECK_IN 1
	move.l	d5,d6
	and.b	#$1f,d6
	move.b	(a5,d6.w),d6			; length = _LZG_LENGTH_DECODE_LUT[b & 0x1f]
	lsl.w	#3,d5
	move.b	(a0)+,d5
	addq.w	#8,d5				; offset = ((b & 0xe0) << 3) | b2
	bra.s	dc7

dc5:	; marker3 - "Short copy"
	BOUNDS_CHECK_IN 1
	moveq	#0,d5
	move.b	(a0)+,d5
	beq	dc2				; Single occurance of the marker symbol (rare)
	move.w	d5,d6
	asr.b	#6,d6
	addq.w	#3,d6				; length = (b >> 6) + 3
	and.b	#$3f,d5
	addq.w	#8,d5				; offset = (b & 0x3f) + 8
	bra.s	dc7

dc6:	; marker4 - "Near copy (incl. RLE)"
	BOUNDS_CHECK_IN 1
	moveq	#0,d5
	move.b	(a0)+,d5
	beq	dc2				; Single occurance of the marker symbol (rare)
	move.w	d5,d6
	and.b	#$1f,d6
	move.b	(a5,d6.w),d6			; length = _LZG_LENGTH_DECODE_LUT[b & 0x1f]
	lsr.b	#5,d5
	addq.w	#1,d5				; offset = (b >> 5) + 1

dc7:	; Copy corresponding data from history window
	move.l	a1,a4
	sub.l	d5,a4
	; FIXME: if (!((copy >= out) && ((dst + length) <= outEnd))) return 0;
	subq.l	#1,d6
dc8:	move.b	(a4)+,(a1)+
	dbf	d6,d8

	cmp.l	a2,a0
	bcs	dc1
	bra.s	_done

_plainCopy:
	move.l	encodedSize(sp),d6
	cmp.l	decodedSize(sp),d6
	bne	_fail
	subq.l	#1,d6
dc9:	move.b	(a0)+,(a1)+
	dbf	d6,dc9

_done:	; We're done
	cmp.l	a3,a1
	bne	_fail
	move.l	decodedSize(sp),d2
	lea.l	STACK_FRAME_SIZE(sp),sp
	movem.l	(sp)+,d0/d1/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4/a5
	rts
	
_fail:	; This is where we end up if something went wrong...
	moveq	#0,d2
	lea.l	STACK_FRAME_SIZE(sp),sp
	movem.l	(sp)+,d0/d1/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4/a5
	rts

