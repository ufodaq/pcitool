#!/bin/bash

echo "Set CH_5 clock ADC 2 delay... "

upfix=000501
fixed=5

    hex_val=$(printf "%01x\n" $1)
    echo "Set $hex_val --> Time value picosecond = `expr $1 "*" 150`."
    pci -w 0x9060 $upfix$hex_val$fixed
