; -------------------------------------------------------------------
; this file is intended to be assembled and linked with the cc65 toolchain.
; It has not been tested with any other assemblers or linkers.
; -------------------------------------------------------------------
; -------------------------------------------------------------------
; example usage do the stream decruncher
; this program will just decrunch some data to memory and print
; the amount of time it took.
; -------------------------------------------------------------------
.import init_decruncher
.import get_decrunched_byte
.export get_crunched_byte
.import end_of_data

.export buffer_start_hi:	absolute
.export buffer_len_hi:		absolute

	.byte $01,$08,$0b,$08,<2003,>2003,$9e,'2','0','6','1',0,0,0
; -------------------------------------------------------------------
; we begin here
; -------------------------------------------------------------------
	sei
	jsr reset_cia1_tod
	inc $01
	jsr init_decruncher
	lda #0
	sta $0400
_sample_next:
	jsr get_decrunched_byte
	bcs _sample_end
	;; do whatever you wish with the value in the accumulator
	ldx store_lo
	bne skip_store_dec_hi
	dec store_hi
skip_store_dec_hi:
	dec store_lo
store_lo = * + 1
store_hi = * + 2
	.byte $8d, 0, 0 		;sta $0000

	dec $01
	sta $d020
	eor $0400
	sta $0400
	inc $01

	jmp _sample_next
_sample_end:
	dec $01

	jsr print_cia1_tod
	cli
	rts
; -------------------------------------------------------------------
; for this get_crunched_byte routine to work the crunched data has to be
; crunced using the -m <buffersize> and possibly the -l flags. Any other
; flag will just mess things up.
get_crunched_byte:
	lda _byte_lo
	bne _byte_skip_hi
	dec _byte_hi
_byte_skip_hi:
	dec _byte_lo
_byte_lo = * + 1
_byte_hi = * + 2
	lda end_of_data		; needs to be set correctly before
	rts			; decrunch_file is called.
; end_of_data needs to point to the address just after the address
; of the last byte of crunched data.
; -------------------------------------------------------------------
reset_cia1_tod:
	lda $dc0e
	ora #$80		; set pal bit
	sta $dc0e

	lda $dc0f
	and #$7f		;  TOD, not alarm mode
	sta $dc0f

	lda #00
	sta $dc0b
	sta $dc0a
	sta $dc09
	sta $dc08
	rts
; -------------------------------------------------------------------
print_cia1_tod:
	lda $dc08 + 3
	sta tabell + 3
	lda $dc08 + 2
	sta tabell + 2
	lda $dc08 + 1
	sta tabell + 1
	lda $dc08
	sta tabell

	;; timmar
	lda tabell + 3
	jsr print_byte

	lda #':'
	jsr $ffd2

	;; minuter
	lda tabell + 2
	jsr print_byte

	lda #':'
	jsr $ffd2

	;; sekunder
	lda tabell + 1
	jsr print_byte

	lda #'.'
	jsr $ffd2

	;; sekunder/10
	lda tabell
	asl
	asl
	asl
	asl
	jsr print_byte

	lda #$0d
	jsr $ffd2
	rts

print_byte:
	pha
	lsr
	lsr
	lsr
	lsr
	tax
	lda hextab,x
	jsr $ffd2
	pla
	and #$0f
	tax
	lda hextab,x
	jsr $ffd2
	rts
hextab:
	.asciiz "0123456789ABCDEF"
tabell:
	.byte 0,0,0,0
; -------------------------------------------------------------------

buffer_len_hi	= 4 		; 1k
unaligned_buffer:
	.res (buffer_len_hi * 256) + 255
buffer_start_hi = (unaligned_buffer + 255) / 256

