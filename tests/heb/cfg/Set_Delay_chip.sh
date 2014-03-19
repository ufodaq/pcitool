#!/bin/bash

    zero=0
    hex_val1=$(printf "%02x\n" $1)
    hex_val2=$(printf "%02x\n" $2)
    hex_val3=$(printf "%02x\n" $3)
    hex_val4=$(printf "%02x\n" $4)

    pci -w 0x9080 $zero$hex_val4$hex_val3$hex_val2$hex_val1
    pci -r 0x9080 -s 1
    sleep 0.5			    
