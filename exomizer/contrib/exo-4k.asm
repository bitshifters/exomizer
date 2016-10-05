;
; Tiny Exomizer-Decruncher especially for 4K-Intros and such V1.2
;
; by Wolfram Sang (Ninja/The Dreams) in 2004 based on the original
; decruncher which is copyright (c) 2002, 2003 by Magnus Lind.
;
; -------------------------------------------------------------------
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
;
; -------------------------------------------------------------------
;
; This source can be assembled with AS1.41 available from the releases-page
; of http://www.the-dreams.de. Porting to other assemblers should be easy.
; The above page is also the place for the latest version of this source.
; For a description of the decruncher in general, see the original
; EXODECRUNCH.S from Magnus Lind.
;
; Here is how to use this one:
;  1) Crunch your file using the "-l auto" option of exomizer.
;  2) Check that the load address does not overlap with this decruncher!
;  3) Remove the last three bytes from that file but keep them in mind.
;     The first byte is the "bitbuf_val". The other two are the "dest_addr".
;  4) Edit the compile-options. Edit the binclude at the end.
;  5) Assemble.
; The resulting file is up to 141 bytes smaller than the standard decruncher.
; Note that it must be started with RUN because of ZP-settings. After
; decrunching IRQs are disabled. $01 is not touched (except for full_ram
; option).
; For further suggestions or comments, contact me: ninja@the-dreams.de
;
; Version history:
;  1.1 - first public release
;  1.2 - 3 bytes shorter

; -------------------------------------------------------------------
; initialization for the assembler
              cpu 6502UNDOC
              page 0

; -------------------------------------------------------------------
; compile options

bitbuf_val    = $08         ; Enter the values you removed from the
dest_addr     = $6000       ; compressed file here.

exec_addr     = $1000       ; where to jump after decrunching. if beyond
                            ; approx. $0910, +3 bytes.

spoil_end     = true        ; if $33 bytes after your decrunched file
                            ; should not be spoiled, +2 bytes
full_ram      = false       ; disables ROMs and I/O, +3 bytes
illegals_ok   = true        ; no illegal opcodes, +1 byte

; -------------------------------------------------------------------
; zero page addresses used. square brackets show the status after
; decrunching.
; -------------------------------------------------------------------

; these can be changed easily

zp_bits_lo    = $61
zp_bits_hi    = $66            ; [undefined, but likely 0]

zp_bit_cnt    = $62            ; [$ff]

; change these only if you really know what you're doing!

zp_len_lo     = $4a            ; $4A = LSR A [undefined]

zp_src_lo     = $78            ; $78 = SEI
zp_src_hi     = zp_src_lo + 1  ; [undefined]

zp_dest_lo    = $39            ; = BASIC Line Number, will be set by RUN
zp_dest_hi    = zp_dest_lo + 1 ; [start address of uncrunched file]

byte_lo       = $2d            ; = end of BASIC-prog
byte_hi       = byte_lo + 1    ; [start address of crunched file]

tabl_bi_pnt   = $b2         ; ptr to tape buffer    (= $033d - $0371)
tabl_lo_pnt   = $75         ; CHRGET-code           (= $02d1 - $0305)
tabl_hi_pnt   = $91         ; STOP-flag + tape temp (= $0100 - $0134)
                            ;  not 100%
; $d1 (ptr to cur_line) might be another idea, but prints on screen...

; -------------------------------------------------------------------
; code starts here

              org $0801

              adr $080d, dest_addr
              byt $9e,"206"
tabl_off:
              byt 48+1,32+1,16+1          ; also part of SYS-line!
              byt 0

              ldy #0
             if full_ram
              sei
              inc $01
             endif

; -------------------------------------------------------------------
; calculate tables (i think it might be possible to gain some bytes here,
; but i don't want to delay the release. suggestions are welcome...)

nextone:
              ldx #1        ; inx? well, AR-reset doesn't clear $030d
              tya
              and #$0f
              beq shortcut
              txa
              lsr a
rolle:
              rol a
              rol zp_bits_hi
              dec zp_bits_lo
              bpl rolle                   ; c = 0

              adc (tabl_lo_pnt),y
              tax

              lda zp_bits_hi
              adc (tabl_hi_pnt),y
shortcut:
              iny
              sta (tabl_hi_pnt),y
              txa
              sta (tabl_lo_pnt),y

             if illegals_ok
              ldx #4
             elseif
              lda #4
             endif
              jsr get_bits
              sta (tabl_bi_pnt),y
              cpy #$34
              bcc nextone
             if spoil_end
              bcs copy_next
             elseif
              ldy #0
              beq next1
             endif

; -------------------------------------------------------------------
; get bits

get_bits:
             if illegals_ok
              stx zp_bit_cnt
             elseif
              sta zp_bit_cnt
             endif

              ldx #0
              stx zp_bits_lo
              stx zp_bits_hi
bitbuf  = * + 1
              lda #bitbuf_val
              asl a
bits_next:
              ror a
              bne ok
get_byte:
              lda byte_lo
              bne byte_skip_hi
              dec byte_hi
byte_skip_hi:
              dec byte_lo
              lda (byte_lo,x)
              bcc literal_entry
              ror a
ok:
              rol zp_bits_lo
              rol zp_bits_hi
              dec zp_bit_cnt
              bpl bits_next

              sta bitbuf
              ldx zp_bits_hi
              lda zp_bits_lo
              rts

tabl_bit:
              byt 2,4,4

; -------------------------------------------------------------------
; main copy loop

copy_next_hi:
              dex
              dec zp_dest_hi
              dec zp_src_hi
copy_next:
              dey
              lda (zp_src_lo),y
             if (full_ram) || (zp_src_lo<>$78)
literal_entry = *
             elseif
literal_entry = * - 1                     ; disables IRQs
             endif
              sta (zp_dest_lo),y
copy_start:
              tya
              bne copy_next
              txa
              bne copy_next_hi
; -------------------------------------------------------------------
; decruncher entry point, needs calculated tables
; y must be #0 when entering

next1:
             if illegals_ok
              inx
             elseif
              lda #1
             endif
              iny
              jsr get_bits
              beq next1
              dey
              beq literal_start

; -------------------------------------------------------------------
; calulate length of sequence (zp_len)

             if illegals_ok
              lax (tabl_bi_pnt),y
             elseif
              lda (tabl_bi_pnt),y
             endif
              jsr get_bits

              cpy #$11
exec_offset = exec_addr - (*+2)
             if exec_offset>127
              bcs start_decrunched
             elseif
              bcs exec_addr
             endif

              adc (tabl_lo_pnt),y
              sta zp_len_lo
              txa
              adc (tabl_hi_pnt),y
              pha
; -------------------------------------------------------------------
; here we decide what offset table to use

              bne nots123
              ldy zp_len_lo
              cpy #$04
              bcc size123
nots123:
              ldy #$03
size123:
             if illegals_ok
              ldx tabl_bit - 1,y
             elseif
              lda tabl_bit - 1,y
             endif
              jsr get_bits
              adc tabl_off - 1,y
              tay
; -------------------------------------------------------------------
; prepare zp_dest

              lda zp_len_lo
literal_start = * - 1
              sbc zp_dest_lo
              eor #$ff
              sta zp_dest_lo
              bcc noborrow
              dec zp_dest_hi
noborrow:
              cpy #1
              bcc get_byte
; -------------------------------------------------------------------
; calulate absolute offset (zp_src)

             if illegals_ok
              lax (tabl_bi_pnt),y
             elseif
              lda (tabl_bi_pnt),y
             endif
              jsr get_bits
              adc (tabl_lo_pnt),y
              bcc skipcarry
              inx
              clc
skipcarry:
              adc zp_dest_lo
              sta zp_src_lo
              txa
              adc (tabl_hi_pnt),y
              adc zp_dest_hi
              sta zp_src_hi
; -------------------------------------------------------------------
; prepare for copy loop

              ldy zp_len_lo
              pla
              tax
              bcc copy_start

; -------------------------------------------------------------------

             if exec_offset>127
start_decrunched:
              jmp exec_addr
             endif
; -------------------------------------------------------------------
; end of decruncher
; -------------------------------------------------------------------

              binclude "c64_src\lib\a.e4k",2

              end $0801
