#!/bin/bash
file=$(basename $1)
filename=${file%.*}.cpp
echo "#define UBRR0H
#include <Arduino.h>
$(cat $1)" > $filename 
echo $filename
make combined file=$filename
sed -i '1,2d' "rose_rose_$filename"
mv "rose_rose_$filename" "$filename.ino"
