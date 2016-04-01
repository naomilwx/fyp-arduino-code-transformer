#!/bin/bash
file=$(basename $1)
dir=$(dirname $1)
filename=${file%.*}_ctmod.cpp
resultdir=$dir/ctResult
mkdir -p $resultdir
echo "#define UBRR0H
#define UBRR1H
#define UBRR2H
#define UBRR3H
#include <Arduino.h>
$(cat $1)" > $dir/$filename
echo $filename
make combined file=$dir/$filename userincl=$2
sed -i '1,5d' "rose_rose_$filename"
mv "rose_rose_$filename" "$resultdir/$filename.ino"
rm $dir/$filename
rm rose_*.cpp
rm ${filename%.*}.o
