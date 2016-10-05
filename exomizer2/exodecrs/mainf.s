; -------------------------------------------------------------------
; this file is intended to be assembled and linked with the cc65 toolchain.
; It has not been tested with any other assemblers or linkers.
; -------------------------------------------------------------------
; -------------------------------------------------------------------
; example usage of Krill's forward decruncher
; this program will just decrunch some data to memory and print
; the amount of time it took.
; -------------------------------------------------------------------
.import decrunch
.export get_crunched_byte
.import start_of_data

	.byte $01,$08,$0b,$08,<2003,>2003,$9e,'2','0','6','1',0,0,0
; -------------------------------------------------------------------
; we begin here
; -------------------------------------------------------------------
	sei
	jsr reset_cia1_tod
	inc $01
	jsr decrunch
	dec $01
	jsr print_cia1_tod
	cli
	rts
; -------------------------------------------------------------------
get_crunched_byte:
	dec $01
	sta $d020
	eor $0400
	sta $0400
	inc $01
_byte_lo = * + 1
_byte_hi = * + 2
	lda start_of_data	; needs to be set correctly before
				; decrunch_file is called.
	inc _byte_lo
	bne _byte_skip_hi
	inc _byte_hi
_byte_skip_hi:
	rts
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
