;################################################
;# Custom Solid Sprite routine
;# Original by imamelia, further enhanced by lx5 & TheBiob
;# 
;# Scratch ram $00-$0F will not be restored?.
;# 
;# input:
;#      A = area width
;#      Y = area height
;#      $00 = multiply factor - 1
;#      Carry = crush flag
;#              clear = crush player if trapped inside
;#              set = don't crush the player if trapped inside
;#        
;# Output:
;#         $8A = Contact information
;#              yx-mrlbt
;#              t = touching top
;#              b = touching bottom
;#              l = touching left side
;#              m = in the middle, set only if crush is off
;#              r = touching right side
;#              x = touching X middle range
;#              y = touching Y middle range

?PlayerInteractRt:
    PHP
    STA $8A
    STY $8B
    JSR ?SubSetInteractBounds
    LDA $45
    CMP $08
    BCC ?.SetLeft
    CMP $0C
    BCC ?.SetMiddleX
?.SetRight
    LDY #$08
    BRA ?.SetXBits
?.SetLeft
    LDY #$04
    BRA ?.SetXBits
?.SetMiddleX
    LDY #$80
?.SetXBits
    STY $8B
    LDA $47
    CMP $0A
    BCC ?.SetTop
    CMP $0E
    BCC ?.SetMiddleY
?.SetBottom
    LDY #$02
    BRA ?.SetYBits
?.SetTop
    LDY #$01
    BRA ?.SetYBits
?.SetMiddleY
    LDY #$40
?.SetYBits
    STY $8C
    SEP #$20
    LDA $8B
    ORA $8C
    STA $8A
    TAY
    LDA !190F,x
    LSR
    BCS ?.Platform
    CPY #$C0
    BEQ ?.CrushInMiddle
    TYA
    LSR
    BCS ?.PosTop
    LSR
    BCS ?.PosBottom
?.TryAgain
    LSR
    BCS ?.PosLeft
    LSR
    BCS ?.PosRight
    PLP : RTL
?.CrushInMiddle
    PLP : BCC ?.SetFlag
    JSL $00F606|!BankB
?.SetFlag
    LDA $8A
    ORA #$10
    STA $8A
    INC 
?.Return
    RTL
?.Platform
    TYA
    LSR
    BCS ?.PosTop
    PLP : RTL
?.PosBottom
    AND #$03
    BNE ?.TryAgain
    LDA #$10
    CLC
    ADC !AA,x
    STA $7D
    LDA #$01
    STA $1DF9|!addr
    PLP : RTL
?.PosLeft
    LDA !E4,x
    SEC
    SBC #$0E
    STA $94
    LDA !14E0,x
    SBC #$00
    STA $95
    LDA $7B
    BEQ ?..Enough
    BMI ?..Enough
    STZ $7B
?..Enough
?..Return
    PLP : RTL
?.PosRight
    LDA $0C
    SEC
    SBC #$02
    STA $94
    LDA $0D
    SBC #$00
    STA $95
    LDA $7B
    BEQ ?..Enough
    BPL ?..Enough
    STZ $7B
?..Enough
?..Return
    PLP : RTL
?.PosTop
    LDA $7D
    BMI ?..Return
    LDA $77
    AND #$08
    BNE ?..Return
    
    LDA #$E0
    LDY $187A|!addr
    BEQ $03
    SEC
    SBC #$10
    LDY.w !AA,x
    BEQ $05
    BMI $03
    CLC
    ADC #$02
    CLC
    ADC !D8,x
    STA $96
    LDA !14D4,x
    ADC #$FF
    STA $97
    LDY #$00
    LDA $1491|!addr
    BPL ?..MovingRight
    DEY
?..MovingRight
    CLC
    ADC $94
    STA $94
    TYA
    ADC $95
    STA $95
    
    LDA #$01
    STA $1471|!addr
    LDA #$10
    STA $7D
    LDA #$80
    STA $1406|!addr
?..Return
    PLP : RTL

; after this:
; - $00-$01 = X position of player hitbox left boundary
; - $02-$03 = Y position of player hitbox top boundary
; - $04-$05 = X position of player hitbox right boundary
; - $06-$07 = Y position of player hitbox bottom boundary
; - $08-$09 = X position of sprite hitbox left boundary
; - $0A-$0B = Y position of sprite hitbox top boundary
; - $0C-$0D = X position of sprite hitbox right boundary
; - $0E-$0F = Y position of sprite hitbox bottom boundary
; - $45-$46 = X position of player hitbox middle boundary
; - $47-$48 = Y position of player hitbox middle boundary
; - $49-$4A = X position of sprite hitbox middle boundary
; - $4B-$4C = Y position of sprite hitbox middle boundary
?SubSetInteractBounds:
    STZ $08
    STZ $09

    LDA #$FE
    STA $0A
    LDA #$FF
    STA $0B
    STZ $0D
    STZ $0F

    LDA $8A
    STA $0C
    LDY $00
    BEQ ?.no_multiply_x
if !sa1
    STZ $2250
    STA $2251
    STZ $2252
    LDA $00
    INC 
    STA $2253
    STZ $2254
    NOP #1
    LDA $2306
    STA $0C
    LDA $2307
    STA $0D
else
    STA $211B
    STZ $211B
    LDA $00
    INC
    STA $211C
    LDA $2134
    STA $0C
    LDA $2135
    STA $0D
endif
?.no_multiply_x

    LDA $8B
    STA $0E
    LDY $00
    BEQ ?.no_multiply_y
if !sa1
    STZ $2250
    STA $2251
    STZ $2252
    LDA $00
    INC 
    STA $2253
    STZ $2254
    NOP #1
    LDA $2306
    STA $0E
    LDA $2307
    STA $0F
else
    STA $211B
    STZ $211B
    LDA $00
    INC
    STA $211C
    LDA $2134
    STA $0E
    LDA $2135
    STA $0F
endif
?.no_multiply_y

    %SetSpriteClippingAlternate()
    %SetPlayerClippingAlternate()
    %CheckForContactAlternate()
    BCC ?.ReturnNoContact
    REP #$20
    LDA $04
    LSR
    CLC
    ADC $00
    STA $45
    LDA $00
    CLC
    ADC $04
    STA $04
    LDA $06
    LSR
    CLC
    ADC $02
    STA $47
    LDA $02
    CLC
    ADC $06
    STA $06
    LDA $0C
    LSR
    CLC
    ADC $08
    STA $49
    LDA $08
    CLC
    ADC $0C
    STA $0C
    LDA $0E
    LSR
    CLC
    ADC $0A
    STA $4B
    LDA $0A
    CLC
    ADC $0E
    STA $0E
    RTS
?.ReturnNoContact
    PLA
    PLA
    LDA #$00
    STA $8A
    PLP : RTL