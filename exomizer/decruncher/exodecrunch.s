;
; Copyright (c) 2002, 2003 Magnus Lind.
;
; This software is provided 'as-is', without any express or implied warranty.
; In no event will the authors be held liable for any damages arising from
; the use of this software.
;
; Permission is granted to anyone to use this software for any purpose,
; including commercial applications, and to alter it and redistribute it
; freely, subject to the following restrictions:
;
;   1. The origin of this software must not be misrepresented; you must not
;   claim that you wrote the original software. If you use this software in a
;   product, an acknowledgment in the product documentation would be
;   appreciated but is not required.
;
;   2. Altered source versions must be plainly marked as such, and must not
;   be misrepresented as being the original software.
;
;   3. This notice may not be removed or altered from any distribution.
;
;   4. The names of this software and/or it's copyright holders may not be
;   used to endorse or promote products derived from this software without
;   specific prior written permission.
;
; -------------------------------------------------------------------
; The decruncher jsr:s to the get_crunched_byte address when it wants to
; read a crunched byte. This subroutine has to preserve x and y register
; and must not modify the state of the carry flag.
; -------------------------------------------------------------------
.import get_crunched_byte
; -------------------------------------------------------------------
; this function is the heart of the decruncher.
; It initializes the decruncher zeropage locations and precalculates the
; decrunch tables and decrunches the data
; This function will not change the interrupt status bit and it will not
; modify the memory configuration.
; -------------------------------------------------------------------
.export decrunch

; -------------------------------------------------------------------
; zero page addresses used
; -------------------------------------------------------------------
zp_len_lo = $a7

zp_src_lo  = $ae
zp_src_hi  = zp_src_lo + 1

zp_bits_hi = $fc

zp_bitbuf  = $fd
zp_dest_lo = zp_bitbuf + 1	; dest addr lo
zp_dest_hi = zp_bitbuf + 2	; dest addr hi

tabl_bi = decrunch_table
tabl_lo = decrunch_table + 52
tabl_hi = decrunch_table + 104

; -------------------------------------------------------------------
; no code below this comment has to be modified in order to generate
; a working decruncher of this source file.
; However, you may want to relocate the tables last in the file to a
; more suitable address.
; -------------------------------------------------------------------

; -------------------------------------------------------------------
; jsr this label to decrunch, it will in turn init the tables and
; call the decruncher
; no constraints on register content, however the
; decimal flag has to be #0 (it almost always is, otherwise do a cld)
decrunch:
; -------------------------------------------------------------------
; init zeropage, x and y regs. (12 bytes)
;
	ldy #0
	ldx #3
init_zp:
	jsr get_crunched_byte
	sta zp_bitbuf - 1,x
	dex
	bne init_zp
; -------------------------------------------------------------------
; calculate tables (50 bytes)
; x and y must be #0 when entering
;
nextone:
	inx
	tya
	and #$0f
	beq shortcut		; starta på ny sekvens

	txa			; this clears reg a
	lsr a			; and sets the carry flag
	ldx tabl_bi-1,y
rolle:
	rol a
	rol zp_bits_hi
	dex
	bpl rolle		; c = 0 after this (rol zp_bits_hi)

	adc tabl_lo-1,y
	tax

	lda zp_bits_hi
	adc tabl_hi-1,y
shortcut:
	sta tabl_hi,y
	txa
	sta tabl_lo,y

	ldx #4
	jsr get_bits		; clears x-reg.
	sta tabl_bi,y
	iny
	cpy #52
	bne nextone
	ldy #0
	beq begin
; -------------------------------------------------------------------
; get bits (29 bytes)
;
; args:
;   x = number of bits to get
; returns:
;   a = #bits_lo
;   x = #0
;   c = 0
;   zp_bits_lo = #bits_lo
;   zp_bits_hi = #bits_hi
; notes:
;   y is untouched
;   other status bits are set to (a == #0)
; -------------------------------------------------------------------
get_bits:
	lda #$00
	sta zp_bits_hi
	cpx #$01
	bcc bits_done
bits_next:
	lsr zp_bitbuf
	bne ok
	pha
literal_get_byte:
	jsr get_crunched_byte
	bcc literal_byte_gotten
	ror a
	sta zp_bitbuf
	pla
ok:
	rol a
	rol zp_bits_hi
	dex
	bne bits_next
bits_done:
	rts
; -------------------------------------------------------------------
; main copy loop (16 bytes)
;
copy_next_hi:
	dex
	dec zp_dest_hi
	dec zp_src_hi
copy_next:
	dey
	lda (zp_src_lo),y
literal_byte_gotten:
	sta (zp_dest_lo),y
copy_start:
	tya
	bne copy_next
begin:
	txa
	bne copy_next_hi
; -------------------------------------------------------------------
; decruncher entry point, needs calculated tables (15 bytes)
; x and y must be #0 when entering
;
	dey
begin2:
	inx
	jsr bits_next
	lsr a
	iny
	bcc begin2
	beq literal_start
	cpy #$11
	bcs bits_done
; -------------------------------------------------------------------
; calulate length of sequence (zp_len) (11 bytes)
;
	ldx tabl_bi - 1,y
	jsr get_bits
	adc tabl_lo - 1,y	; we have now calculated zp_len_lo
	sta zp_len_lo
; -------------------------------------------------------------------
; now do the hibyte of the sequence length calculation (6 bytes)
	lda zp_bits_hi
	adc tabl_hi - 1,y	; c = 0 after this.
	pha
; -------------------------------------------------------------------
; here we decide what offset table to use (20 bytes)
; x is 0 here
;
	bne nots123
	ldy zp_len_lo
	cpy #$04
	bcc size123
nots123:
	ldy #$03
size123:
	ldx tabl_bit - 1,y
	jsr get_bits
	adc tabl_off - 1,y	; c = 0 after this.
	tay			; 1 <= y <= 50 here
; -------------------------------------------------------------------
; Here we do the dest_lo -= len_lo subtraction to prepare zp_dest
; but we do it backwards:	a - b == (b - a - 1) ^ ~0 (C-syntax)
; (14 bytes)
	lda zp_len_lo
literal_start:			; literal enters here with y = 0, c = 1
	sbc zp_dest_lo
	bcc noborrow
	dec zp_dest_hi
noborrow:
	eor #$ff
	sta zp_dest_lo
	cpy #$01		; y < 1 then literal
	bcc literal_get_byte
; -------------------------------------------------------------------
; calulate absolute offset (zp_src) (27 bytes)
;
	ldx tabl_bi,y
	jsr get_bits;
	adc tabl_lo,y
	bcc skipcarry
	inc zp_bits_hi
	clc
skipcarry:
	adc zp_dest_lo
	sta zp_src_lo
	lda zp_bits_hi
	adc tabl_hi,y
	adc zp_dest_hi
	sta zp_src_hi
; -------------------------------------------------------------------
; prepare for copy loop (6 bytes)
;
	pla
	tax
	ldy zp_len_lo
	bcc copy_start
; -------------------------------------------------------------------
; two small static tables (6 bytes)
;
tabl_bit:
	.byte 2,4,4
tabl_off:
	.byte 48,32,16
; -------------------------------------------------------------------
; end of decruncher
; -------------------------------------------------------------------

; -------------------------------------------------------------------
; this 156 byte table area may be relocated. It may also be clobbered
; by other data between decrunches.
; -------------------------------------------------------------------
decrunch_table:
	.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0,0,0
; -------------------------------------------------------------------
; end of decruncher
; -------------------------------------------------------------------
