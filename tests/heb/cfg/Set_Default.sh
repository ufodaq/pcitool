#!/bin/bash

echo "Set Defaults delay value in the board... "

./Set_FPGA_clock_delay.sh 0
sleep 0.1

 ./Set_Delay_chip.sh 16 16 16 16
sleep 0.1

./Set_TH_Delay.sh 12
sleep 0.1

./Set_ADC_1_Delay.sh 5
sleep 0.1

./Set_ADC_2_Delay.sh 5
sleep 0.1

./Set_ADC_3_Delay.sh 5
sleep 0.1

./Set_ADC_4_Delay.sh 5

#pci -w 0x9020 200b20
pci -w 0x9020 20
pci -w 0x9028 e

echo " DONE ................ "

