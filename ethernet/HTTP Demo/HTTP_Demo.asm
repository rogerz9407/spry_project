
_EXPANDER_write:

	PUSH	W10
	CALL	_I2C2_Start
	CALL	_I2C2_Write
	MOV.B	W11, W10
	CALL	_I2C2_Write
	MOV.B	W12, W10
	CALL	_I2C2_Write
	CALL	_I2C2_Stop
L_end_EXPANDER_write:
	POP	W10
	RETURN
; end of _EXPANDER_write

_EXPANDER_read:
	LNK	#2

	PUSH	W10
	CALL	_I2C2_Start
	CALL	_I2C2_Write
	PUSH	W10
	MOV.B	W11, W10
	CALL	_I2C2_Write
	POP	W10
	CALL	_I2C2_Restart
	ZE	W10, W0
	INC	W0
	MOV.B	W0, W10
	CALL	_I2C2_Write
	MOV	#1, W10
	CALL	_I2C2_Read
	MOV.B	W0, [W14+0]
	CALL	_I2C_Stop
	MOV.B	[W14+0], W0
L_end_EXPANDER_read:
	POP	W10
	ULNK
	RETURN
; end of _EXPANDER_read

_EXPANDER_init:

	PUSH	W10
	PUSH	W11
	PUSH	W12
	PUSH	W12
	PUSH	W11
	MOV.B	W10, W12
	CLR	W11
	MOV.B	#64, W10
	CALL	_EXPANDER_write
	POP	W11
	MOV.B	W11, W12
	MOV.B	#1, W11
	MOV.B	#64, W10
	CALL	_EXPANDER_write
	POP	W12
	CLR	W11
	MOV.B	#66, W10
	CALL	_EXPANDER_write
	MOV.B	#255, W12
	MOV.B	#1, W11
	MOV.B	#66, W10
	CALL	_EXPANDER_write
L_end_EXPANDER_init:
	POP	W12
	POP	W11
	POP	W10
	RETURN
; end of _EXPANDER_init

_INIT1_interrupt:

	PUSH	W10
	PUSH	W11
	PUSH	W12
	MOV.B	#255, W12
	MOV.B	#2, W11
	MOV.B	#66, W10
	CALL	_EXPANDER_write
	CLR	W12
	MOV.B	#3, W11
	MOV.B	#66, W10
	CALL	_EXPANDER_write
	CLR	W12
	MOV.B	#4, W11
	MOV.B	#66, W10
	CALL	_EXPANDER_write
L_end_INIT1_interrupt:
	POP	W12
	POP	W11
	POP	W10
	RETURN
; end of _INIT1_interrupt

_I2C_int1Chip2:
	PUSH	DSWPAG
	PUSH	50
	PUSH	RCOUNT
	PUSH	W0
	MOV	#2, W0
	REPEAT	#12
	PUSH	[W0++]

	MOV	#lo_addr(_NI2CD), W1
	MOV.B	#1, W0
	MOV.B	W0, [W1]
	BCLR	IFS1, #4
L_end_I2C_int1Chip2:
	MOV	#26, W0
	REPEAT	#12
	POP	[W0--]
	POP	W0
	POP	RCOUNT
	POP	50
	POP	DSWPAG
	RETFIE
; end of _I2C_int1Chip2

_bank_3select:

	PUSH	W10
	BCLR	LATGbits, #0
	MOV	#191, W10
	CALL	_SPI1_Write
	MOV	#255, W10
	CALL	_SPI1_Write
	BSET	LATGbits, #0
	BCLR	LATGbits, #0
	MOV	#159, W10
	CALL	_SPI1_Write
	MOV	#3, W10
	CALL	_SPI1_Write
	BSET	LATGbits, #0
L_end_bank_3select:
	POP	W10
	RETURN
; end of _bank_3select

_int_mcu:

	MOV	#992, W0
	MOV	WREG, OSCCON
	MOV	#6, W0
	MOV	WREG, CLKDIV
	MOV	#278, W0
	MOV	WREG, PLLFBD
	CLR	ANSELD
	CLR	ANSELA
L_end_int_mcu:
	RETURN
; end of _int_mcu

_SPI1_pin_config:

	BCLR	TRISG, #0
	BCLR	TRISGbits, #14
	BCLR	TRISEbits, #0
	BSET	TRISEbits, #1
	BCLR	TRISGbits, #1
	BCLR	ANSELG, #0
	BCLR	ANSELEbits, #1
L_end_SPI1_pin_config:
	RETURN
; end of _SPI1_pin_config

_map_io:

	PUSH	W10
	PUSH	W11
	PUSH	W12
	CALL	_Unlock_IOLOCK
	MOV.B	#5, W12
	CLR	W11
	MOV.B	#126, W10
	CALL	_PPS_Mapping_NoLock
	MOV.B	#6, W12
	CLR	W11
	MOV.B	#80, W10
	CALL	_PPS_Mapping_NoLock
	MOV.B	#38, W12
	MOV.B	#1, W11
	MOV.B	#81, W10
	CALL	_PPS_Mapping_NoLock
	CALL	_Lock_IOLOCK
L_end_map_io:
	POP	W12
	POP	W11
	POP	W10
	RETURN
; end of _map_io

_SPI1_regi_config:

	MOV	#288, W0
	MOV	WREG, SPI1CON1
	MOV	#32768, W0
	MOV	WREG, SPI1STAT
	CLR	SPI1CON2
L_end_SPI1_regi_config:
	RETURN
; end of _SPI1_regi_config

_initialize_spi1_module:

	CALL	_SPI1_pin_config
	CALL	_map_io
	CALL	_SPI1_regi_config
L_end_initialize_spi1_module:
	RETURN
; end of _initialize_spi1_module

_SPI_Ethernet_UserTCP:
	LNK	#2

; flags start address is: 0 (W0)
	MOV	[W14-8], W0
; flags end address is: 0 (W0)
	MOV	#80, W0
	CP	W12, W0
	BRA NZ	L__SPI_Ethernet_UserTCP24
	GOTO	L_SPI_Ethernet_UserTCP0
L__SPI_Ethernet_UserTCP24:
	CLR	W0
	GOTO	L_end_SPI_Ethernet_UserTCP
L_SPI_Ethernet_UserTCP0:
; len start address is: 4 (W2)
	CLR	W2
; len end address is: 4 (W2)
L_SPI_Ethernet_UserTCP1:
; len start address is: 4 (W2)
	CP	W2, #15
	BRA LTU	L__SPI_Ethernet_UserTCP25
	GOTO	L_SPI_Ethernet_UserTCP2
L__SPI_Ethernet_UserTCP25:
	MOV	#lo_addr(_getRequest), W0
	ADD	W0, W2, W0
	MOV	W0, [W14+0]
	PUSH	W2
	PUSH.D	W12
	PUSH.D	W10
	CALL	_SPI_Ethernet_getByte
	POP.D	W10
	POP.D	W12
	POP	W2
	MOV	[W14+0], W1
	MOV.B	W0, [W1]
	INC	W2
	GOTO	L_SPI_Ethernet_UserTCP1
L_SPI_Ethernet_UserTCP2:
	MOV	#lo_addr(_getRequest), W0
	ADD	W0, W2, W1
; len end address is: 4 (W2)
	CLR	W0
	MOV.B	W0, [W1]
	PUSH	W12
	PUSH.D	W10
	MOV	#5, W12
	MOV	#lo_addr(?lstr1_HTTP_Demo), W11
	MOV	#lo_addr(_getRequest), W10
	CALL	_memcmp
	POP.D	W10
	POP	W12
	CP0	W0
	BRA NZ	L__SPI_Ethernet_UserTCP26
	GOTO	L_SPI_Ethernet_UserTCP4
L__SPI_Ethernet_UserTCP26:
	CLR	W0
	GOTO	L_end_SPI_Ethernet_UserTCP
L_SPI_Ethernet_UserTCP4:
	PUSH	W12
	PUSH.D	W10
	MOV	#2, W12
	MOV	#lo_addr(?lstr2_HTTP_Demo), W11
	MOV	#lo_addr(_getRequest+11), W10
	CALL	_memcmp
	POP.D	W10
	POP	W12
	CP0	W0
	BRA Z	L__SPI_Ethernet_UserTCP27
	GOTO	L_SPI_Ethernet_UserTCP5
L__SPI_Ethernet_UserTCP27:
	PUSH	W12
	PUSH.D	W10
	MOV.B	#128, W12
	MOV.B	#20, W11
	MOV.B	#64, W10
	CALL	_EXPANDER_write
	POP.D	W10
	POP	W12
	GOTO	L_SPI_Ethernet_UserTCP6
L_SPI_Ethernet_UserTCP5:
	PUSH	W12
	PUSH.D	W10
	MOV	#3, W12
	MOV	#lo_addr(?lstr3_HTTP_Demo), W11
	MOV	#lo_addr(_getRequest+11), W10
	CALL	_memcmp
	POP.D	W10
	POP	W12
	CP0	W0
	BRA Z	L__SPI_Ethernet_UserTCP28
	GOTO	L_SPI_Ethernet_UserTCP7
L__SPI_Ethernet_UserTCP28:
	PUSH	W12
	PUSH.D	W10
	CLR	W12
	MOV.B	#20, W11
	MOV.B	#64, W10
	CALL	_EXPANDER_write
	POP.D	W10
	POP	W12
	BTSS	PORTB, #0
	GOTO	L_SPI_Ethernet_UserTCP8
	PUSH	W12
	PUSH.D	W10
	MOV	#6, W12
	MOV	#lo_addr(?lstr4_HTTP_Demo), W11
	MOV	#lo_addr(_indexPage+340), W10
	CALL	_memcpy
	MOV	#6, W12
	MOV	#lo_addr(?lstr5_HTTP_Demo), W11
	MOV	#lo_addr(_indexPage+431), W10
	CALL	_memcpy
	POP.D	W10
	POP	W12
	GOTO	L_SPI_Ethernet_UserTCP9
L_SPI_Ethernet_UserTCP8:
	PUSH	W12
	PUSH.D	W10
	MOV	#6, W12
	MOV	#lo_addr(?lstr6_HTTP_Demo), W11
	MOV	#lo_addr(_indexPage+340), W10
	CALL	_memcpy
	MOV	#6, W12
	MOV	#lo_addr(?lstr7_HTTP_Demo), W11
	MOV	#lo_addr(_indexPage+431), W10
	CALL	_memcpy
	POP.D	W10
	POP	W12
L_SPI_Ethernet_UserTCP9:
L_SPI_Ethernet_UserTCP7:
L_SPI_Ethernet_UserTCP6:
	PUSH.D	W12
	PUSH.D	W10
	MOV	#lo_addr(_httpHeader), W10
	CALL	_SPI_Ethernet_putConstString
	POP.D	W10
	POP.D	W12
; len start address is: 2 (W1)
	MOV	W0, W1
	PUSH	W1
	PUSH.D	W12
	PUSH.D	W10
	MOV	#lo_addr(_httpMimeTypeHTML), W10
	CALL	_SPI_Ethernet_putConstString
	POP.D	W10
	POP.D	W12
	POP	W1
	ADD	W1, W0, W1
	PUSH	W1
	PUSH.D	W12
	PUSH.D	W10
	MOV	#lo_addr(_indexPage), W10
	CALL	_SPI_Ethernet_putString
	POP.D	W10
	POP.D	W12
	POP	W1
	ADD	W1, W0, W0
; len end address is: 2 (W1)
L_end_SPI_Ethernet_UserTCP:
	ULNK
	RETURN
; end of _SPI_Ethernet_UserTCP

_SPI_Ethernet_UserUDP:
	LNK	#0

; flags start address is: 0 (W0)
	MOV	[W14-8], W0
; flags end address is: 0 (W0)
	CLR	W0
L_end_SPI_Ethernet_UserUDP:
	ULNK
	RETURN
; end of _SPI_Ethernet_UserUDP

_Mac_reaD:

	PUSH	W10
	CALL	_bank_3select
	BCLR	LATGbits, #0
	MOV	#1, W10
	CALL	_SPI1_Write
	MOV	#lo_addr(_temp), W0
	ZE	[W0], W10
	CALL	_SPI1_Write
	BSET	LATGbits, #0
	BCLR	LATGbits, #0
	CLR	W10
	CALL	_SPI1_Write
	MOV	#lo_addr(_temp), W0
	ZE	[W0], W10
	CALL	_SPI1_Write
	BSET	LATGbits, #0
	BCLR	LATGbits, #0
	MOV	#3, W10
	CALL	_SPI1_Write
	MOV	#lo_addr(_temp), W0
	ZE	[W0], W10
	CALL	_SPI1_Write
	BSET	LATGbits, #0
	BCLR	LATGbits, #0
	MOV	#2, W10
	CALL	_SPI1_Write
	MOV	#lo_addr(_temp), W0
	ZE	[W0], W10
	CALL	_SPI1_Write
	BSET	LATGbits, #0
	BCLR	LATGbits, #0
	MOV	#5, W10
	CALL	_SPI1_Write
	MOV	#lo_addr(_temp), W0
	ZE	[W0], W10
	CALL	_SPI1_Write
	BSET	LATGbits, #0
	BCLR	LATGbits, #0
	MOV	#4, W10
	CALL	_SPI1_Write
	MOV	#lo_addr(_temp), W0
	ZE	[W0], W10
	CALL	_SPI1_Write
	BSET	LATGbits, #0
L_end_Mac_reaD:
	POP	W10
	RETURN
; end of _Mac_reaD

_main:
	MOV	#2048, W15
	MOV	#6142, W0
	MOV	WREG, 32
	MOV	#1, W0
	MOV	WREG, 50
	MOV	#4, W0
	IOR	68

	PUSH	W10
	PUSH	W11
	PUSH	W12
	CALL	_int_mcu
	CALL	_initialize_spi1_module
	MOV	#lo_addr(_SPI1_Write), W11
	MOV	#lo_addr(_SPI1_Read), W10
	CALL	_SPI_Set_Active
	MOV.B	#1, W12
	MOV	#lo_addr(_IpAddr), W11
	MOV	#lo_addr(_MACAddr), W10
	CALL	_SPI_Ethernet_Init
	MOV	#lo_addr(_dnsIpAddr), W12
	MOV	#lo_addr(_gwIpAddr), W11
	MOV	#lo_addr(_ipMask), W10
	CALL	_SPI_Ethernet_confNetwork
	MOV.B	#161, W10
	CALL	_SPI_Ethernet_Enable
	MOV	#34464, W10
	MOV	#1, W11
	CALL	_I2C2_Init
	MOV.B	#255, W12
	MOV.B	#255, W11
	CLR	W10
	CALL	_EXPANDER_init
L_main10:
	CALL	_SPI_Ethernet_doPacket
	GOTO	L_main10
L_end_main:
	POP	W12
	POP	W11
	POP	W10
L__main_end_loop:
	BRA	L__main_end_loop
; end of _main
