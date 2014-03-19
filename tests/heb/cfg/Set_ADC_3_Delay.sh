#!/bin/bash

echo "Set CH_6 clock ADC 3 delay... "

upfix=000501
fixed=6

    hex_val=$(printf "%01x\n" $1)
    echo "Set $hex_val --> Time value picosecond = `expr $1 "*" 150`."
    pci -w 0x9060 $upfix$hex_val$fixed
