#!/bin/bash

echo "Set delay on T/Hs signals... "

upfix=000501
fixed=3

    hex_val=$(printf "%01x\n" $1)
    echo "Set $hex_val --> Time picosecond = `expr $1 "*" 150`."
    pci -w 0x9060 $upfix$hex_val$fixed
