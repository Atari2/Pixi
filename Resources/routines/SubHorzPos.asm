;; Name: SubHorzPos
;; Author: Unspecified at the time of writing 
;; Input: None
;; Output: Y   = 0 => Mario to the right of the sprite,
;;               1 => Mario being on the left.
;;         $0E = 16 bit difference beween Mario X Pos - Sprite X Pos.
;; Clobbers: None
;; Description: Checks if a sprite is on the left or the right of mario
;;              Some sprites use a version of this routine where only $0F is being stored to as the low byte of the distance (here it is the high byte)

	LDY #$00
	LDA $94
	SEC
	SBC !E4,x
	STA $0E
	LDA $95
	SBC !14E0,x
	STA $0F
	BPL ?+
	INY
?+
	RTL