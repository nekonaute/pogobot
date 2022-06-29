EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 14 16
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Device:Q_Photo_NPN Q?
U 1 1 5E5DB215
P 6100 3350
AR Path="/5E5DB215" Ref="Q?"  Part="1" 
AR Path="/5E5DA840/5E5DB215" Ref="Q?"  Part="1" 
AR Path="/5E5DB395/5E5DB215" Ref="Q?"  Part="1" 
AR Path="/5E5DB3D0/5E5DB215" Ref="Q?"  Part="1" 
AR Path="/5E5DB3E9/5E5DB215" Ref="Q?"  Part="1" 
AR Path="/611DA2E5/5E5DB215" Ref="Q?"  Part="1" 
AR Path="/611DA2E8/5E5DB215" Ref="Q?"  Part="1" 
AR Path="/611DA2EB/5E5DB215" Ref="Q?"  Part="1" 
AR Path="/616EDAAF/6171ACB1/5E5DB215" Ref="Q10"  Part="1" 
AR Path="/616EDAAF/6171ACB4/5E5DB215" Ref="Q11"  Part="1" 
AR Path="/616EDAAF/6171ACB7/5E5DB215" Ref="Q12"  Part="1" 
F 0 "Q12" H 6290 3396 50  0000 L CNN
F 1 "Q_Photo_NPN" H 6290 3305 50  0000 L CNN
F 2 "MyFootprints:SFH3710" H 6300 3450 50  0001 C CNN
F 3 "https://www.osram.com/media/resource/hires/osram-dam-5961394/SFH+3710_EN.pdf" H 6100 3350 50  0001 C CNN
F 4 "Osram" H 6100 3350 50  0001 C CNN "Manufacturer"
F 5 "SFH 3710-Z" H 6100 3350 50  0001 C CNN "MPN"
	1    6100 3350
	1    0    0    -1  
$EndComp
$Comp
L Device:R R?
U 1 1 5E5DB21C
P 6200 3800
AR Path="/5E5DB21C" Ref="R?"  Part="1" 
AR Path="/5E5DA840/5E5DB21C" Ref="R?"  Part="1" 
AR Path="/5E5DB395/5E5DB21C" Ref="R?"  Part="1" 
AR Path="/5E5DB3D0/5E5DB21C" Ref="R?"  Part="1" 
AR Path="/5E5DB3E9/5E5DB21C" Ref="R?"  Part="1" 
AR Path="/611DA2E5/5E5DB21C" Ref="R?"  Part="1" 
AR Path="/611DA2E8/5E5DB21C" Ref="R?"  Part="1" 
AR Path="/611DA2EB/5E5DB21C" Ref="R?"  Part="1" 
AR Path="/616EDAAF/6171ACB1/5E5DB21C" Ref="R25"  Part="1" 
AR Path="/616EDAAF/6171ACB4/5E5DB21C" Ref="R26"  Part="1" 
AR Path="/616EDAAF/6171ACB7/5E5DB21C" Ref="R27"  Part="1" 
F 0 "R27" H 6270 3846 50  0000 L CNN
F 1 "1M" H 6270 3755 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 6130 3800 50  0001 C CNN
F 3 "~" H 6200 3800 50  0001 C CNN
	1    6200 3800
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR?
U 1 1 5E5DB229
P 6200 3050
AR Path="/5E5DB229" Ref="#PWR?"  Part="1" 
AR Path="/5E5DA840/5E5DB229" Ref="#PWR?"  Part="1" 
AR Path="/5E5DB395/5E5DB229" Ref="#PWR?"  Part="1" 
AR Path="/5E5DB3D0/5E5DB229" Ref="#PWR?"  Part="1" 
AR Path="/5E5DB3E9/5E5DB229" Ref="#PWR?"  Part="1" 
AR Path="/611DA2E5/5E5DB229" Ref="#PWR?"  Part="1" 
AR Path="/611DA2E8/5E5DB229" Ref="#PWR?"  Part="1" 
AR Path="/611DA2EB/5E5DB229" Ref="#PWR?"  Part="1" 
AR Path="/616EDAAF/6171ACB1/5E5DB229" Ref="#PWR093"  Part="1" 
AR Path="/616EDAAF/6171ACB4/5E5DB229" Ref="#PWR095"  Part="1" 
AR Path="/616EDAAF/6171ACB7/5E5DB229" Ref="#PWR097"  Part="1" 
F 0 "#PWR097" H 6200 2900 50  0001 C CNN
F 1 "+3.3V" H 6215 3223 50  0000 C CNN
F 2 "" H 6200 3050 50  0001 C CNN
F 3 "" H 6200 3050 50  0001 C CNN
	1    6200 3050
	1    0    0    -1  
$EndComp
Wire Wire Line
	6200 3050 6200 3150
Wire Wire Line
	6200 3550 6200 3600
Wire Wire Line
	6200 3950 6200 4150
Text HLabel 6700 3600 2    50   Output ~ 0
Output
Wire Wire Line
	6700 3600 6200 3600
Connection ~ 6200 3600
Wire Wire Line
	6200 3600 6200 3650
$Comp
L power:GND #PWR094
U 1 1 610B3230
P 6200 4150
AR Path="/616EDAAF/6171ACB1/610B3230" Ref="#PWR094"  Part="1" 
AR Path="/616EDAAF/6171ACB4/610B3230" Ref="#PWR096"  Part="1" 
AR Path="/616EDAAF/6171ACB7/610B3230" Ref="#PWR098"  Part="1" 
F 0 "#PWR098" H 6200 3900 50  0001 C CNN
F 1 "GND" H 6205 3977 50  0000 C CNN
F 2 "" H 6200 4150 50  0001 C CNN
F 3 "" H 6200 4150 50  0001 C CNN
	1    6200 4150
	1    0    0    -1  
$EndComp
$EndSCHEMATC
