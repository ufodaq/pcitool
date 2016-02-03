#! /bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 [ xml_file ]"
    exit 0
fi

cat $1 | xmllint -format - | xsltproc format.xsl -
