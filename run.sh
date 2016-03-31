#!/bin/bash
file=$(basename $1)
dir=$(dirname $1)
filename=${file%.*}_ctmod.cpp
resultdir=$dir/ctResult
mkdir -p $resultdir
echo "#define UBRR0H
#include <Arduino.h>
$(cat $1)" > $dir/$filename
echo $filename
make combined file=$dir/$filename
sed -i '1,2d' "rose_rose_$filename"
mv "rose_rose_$filename" "$resultdir/$filename.ino"
rm $dir/$filename
rm rose_*.cpp
