#!/bin/bash

hexdump -e ' "0x%08.8_ax: " 4/4 " 0x%08x " "\n" ' $1 | less
