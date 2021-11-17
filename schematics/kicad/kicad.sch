EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Open-Heat ESP12-E schematic"
Date "2021-11-06"
Rev "0.1"
Comp "Alexander Mohr"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L RF_Module:ESP-12E U?
U 1 1 6179B910
P 5550 4250
F 0 "U?" H 5550 5231 50  0000 C CNN
F 1 "ESP-12E" H 5550 5140 50  0000 C CNN
F 2 "RF_Module:ESP-12E" H 5550 4250 50  0001 C CNN
F 3 "http://wiki.ai-thinker.com/_media/esp8266/esp8266_series_modules_user_manual_v1.1.pdf" H 5200 4350 50  0001 C CNN
	1    5550 4250
	1    0    0    -1  
$EndComp
$Comp
L Motor:Motor_DC M?
U 1 1 6179D18B
P 6850 4650
F 0 "M?" H 6692 4554 50  0000 R CNN
F 1 "Motor_DC" H 6692 4645 50  0000 R CNN
F 2 "" H 6850 4560 50  0001 C CNN
F 3 "~" H 6850 4560 50  0001 C CNN
	1    6850 4650
	-1   0    0    1   
$EndComp
Wire Wire Line
	6150 4250 6850 4250
Wire Wire Line
	6850 4250 6850 4350
Wire Wire Line
	6850 4850 6500 4850
Wire Wire Line
	6500 4850 6500 4450
Wire Wire Line
	6500 4450 6150 4450
Wire Wire Line
	5550 4950 5550 5150
Wire Wire Line
	6150 4350 6700 4350
Wire Wire Line
	6700 3850 6700 4350
$Comp
L SensorCopy:BME280_I2C U?
U 1 1 617D3D91
P 8350 3750
F 0 "U?" H 7920 3796 50  0000 R CNN
F 1 "BME280_I2C" H 7920 3705 50  0000 R CNN
F 2 "Connector_Wire:SolderWirePad_1x04_P3.81mm_Drill1.2mm" H 9850 3300 50  0001 C CNN
F 3 "" H 8350 3550 50  0001 C CNN
	1    8350 3750
	-1   0    0    -1  
$EndComp
Wire Wire Line
	6700 3850 7750 3850
Wire Wire Line
	7750 4050 7750 5150
Wire Wire Line
	6600 3650 6600 4150
Wire Wire Line
	6600 4150 6150 4150
Wire Wire Line
	6600 3650 7750 3650
Wire Wire Line
	6150 4050 6500 4050
Wire Wire Line
	6500 4050 6500 3450
Wire Wire Line
	6500 3450 7750 3450
Wire Wire Line
	4950 3650 4300 3650
Wire Wire Line
	4300 3650 4300 5050
Wire Wire Line
	4300 5050 6250 5050
Wire Wire Line
	6250 5050 6250 4650
Wire Wire Line
	6250 4650 6150 4650
$Comp
L Device:LED D?
U 1 1 617DB3B0
P 5700 5550
F 0 "D?" H 5693 5766 50  0000 C CNN
F 1 "LED" H 5693 5675 50  0000 C CNN
F 2 "" H 5700 5550 50  0001 C CNN
F 3 "~" H 5700 5550 50  0001 C CNN
	1    5700 5550
	1    0    0    -1  
$EndComp
$Comp
L Device:R R120
U 1 1 617E0BE6
P 6000 5550
F 0 "R120" V 5793 5550 50  0000 C CNN
F 1 "R" V 5884 5550 50  0000 C CNN
F 2 "" V 5930 5550 50  0001 C CNN
F 3 "~" H 6000 5550 50  0001 C CNN
	1    6000 5550
	0    1    1    0   
$EndComp
Wire Wire Line
	5550 5550 5550 5150
Connection ~ 5550 5150
Wire Wire Line
	5550 5150 7750 5150
Wire Wire Line
	6150 4550 6350 4550
Wire Wire Line
	6350 4550 6350 5550
Wire Wire Line
	6350 5550 6150 5550
$Comp
L ht7333-a:HT7333-A L?
U 1 1 617A55A2
P 5400 2600
F 0 "L?" H 5450 2997 60  0000 C CNN
F 1 "HT7333-A" H 5450 2891 60  0000 C CNN
F 2 "" H 5400 2600 60  0000 C CNN
F 3 "" H 5400 2600 60  0000 C CNN
	1    5400 2600
	1    0    0    -1  
$EndComp
Wire Wire Line
	5900 2450 5900 3450
Wire Wire Line
	5900 3450 5550 3450
Wire Wire Line
	5550 2900 4200 2900
Wire Wire Line
	4200 2900 4200 4600
Wire Wire Line
	4200 5150 5550 5150
$Comp
L power:+3.3V #PWR?
U 1 1 617A7405
P 4800 2450
F 0 "#PWR?" H 4800 2300 50  0001 C CNN
F 1 "+3.3V" H 4815 2623 50  0000 C CNN
F 2 "" H 4800 2450 50  0001 C CNN
F 3 "" H 4800 2450 50  0001 C CNN
	1    4800 2450
	1    0    0    -1  
$EndComp
Wire Wire Line
	3950 3900 3950 4050
$Comp
L Device:R R3.3M
U 1 1 6186F150
P 3950 4450
F 0 "R3.3M" H 4020 4496 50  0000 L CNN
F 1 "R" H 4020 4405 50  0000 L CNN
F 2 "" V 3880 4450 50  0001 C CNN
F 3 "~" H 3950 4450 50  0001 C CNN
	1    3950 4450
	1    0    0    -1  
$EndComp
$Comp
L Device:R R10M
U 1 1 6186E03C
P 3950 3750
F 0 "R10M" H 4020 3796 50  0000 L CNN
F 1 "R" H 4020 3705 50  0000 L CNN
F 2 "" V 3880 3750 50  0001 C CNN
F 3 "~" H 3950 3750 50  0001 C CNN
	1    3950 3750
	1    0    0    -1  
$EndComp
Wire Wire Line
	4950 4050 3950 4050
Wire Wire Line
	3950 4300 3950 4050
Connection ~ 3950 4050
Wire Wire Line
	3950 2600 3950 3600
Wire Wire Line
	3950 2600 4800 2600
Wire Wire Line
	4800 2450 4800 2600
Connection ~ 4800 2600
Wire Wire Line
	4800 2600 5000 2600
Wire Wire Line
	3950 4600 4200 4600
Connection ~ 4200 4600
Wire Wire Line
	4200 4600 4200 5150
$EndSCHEMATC
