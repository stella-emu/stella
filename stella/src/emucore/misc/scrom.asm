;;============================================================================
;;
;;   SSSS    tt          lll  lll
;;  SS  SS   tt           ll   ll
;;  SS     tttttt  eeee   ll   ll   aaaa
;;   SSSS    tt   ee  ee  ll   ll      aa
;;      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
;;  SS  SS   tt   ee      ll   ll  aa  aa
;;   SSSS     ttt  eeeee llll llll  aaaaa
;;
;; Copyright (c) 1995-1998 by Bradford W. Mott
;;
;; See the file "license" for information on usage and redistribution of
;; this file, and for a DISCLAIMER OF ALL WARRANTIES.
;;
;; $Id: scrom.asm,v 1.1.1.1 2001-12-27 19:54:32 bwmott Exp $
;;============================================================================
;; 
;; This file contains a "dummy" Supercharger ROM for Stella.  It basically
;; contains some bootstrapping code to get the game up and running.
;;
;;============================================================================

        processor 6502

        org $FA00      

;;
;; Normal clear page zero routine for initial program load
;;
        LDA #0
        LDX #0
clear   STA $80,X
        INX
        CPX #$80
        BNE clear
        JMP cpcode

;;
;; Clear page zero routine for multi-load
;;
        org $FA20

        LDA #0
        LDX #0
mlclr   STA $80,X
        INX
        CPX #$1E
        BNE mlclr

;;
;; Now, copy some code into page zero to do the initial bank switch
;;
cpcode  LDX #0
copy    LDA code,X
        STA $fa,X
        INX
        CPX #6
        BNE copy

;;
;; Initialize X and Y registers
;;
        LDX #$ff
        LDY #$00

;;
;; Store the bank configuration in $80 and in CMP instruction
;;
        LDA #$00    ;; $00 is changed by emulator to the correct value
        STA $80

        CMP $f000   ;; $00 is changed by emulator to the correct value
;;
;; Execute the code to do bank switch and start running cartridge code
;;
        JMP $fa

code    dc.b $ad, $f8, $ff    ;; LDA $fff8
        dc.b $4c, $00, $00    ;; JMP $???? ($???? is filled in by emulator)

