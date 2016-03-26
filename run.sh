#!/bin/bash
file=$(basename $1)
filename=${file%.*}.cpp
echo "#define UBRR0H
#include <Arduino.h>
$(cat $1)" > $filename 
echo $filename
make combined file=$filename
