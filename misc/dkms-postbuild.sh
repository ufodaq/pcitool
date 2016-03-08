#! /bin/bash

make -C driver/ install_symvers KERNELVER=$kernelver KERNELDIR=$kernel_source_dir
