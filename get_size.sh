#!/bin/bash
set -e
file=$(basename $1)
dir=$(dirname $1)
filename=${file%.*}_ctmod.cpp
resultdir=$dir/ctResult
mkdir -p $resultdir
echo "#define	prog_char	char
#define	PGM_P	char *
#include <Arduino.h>
$(cat $1)" > $dir/$filename
echo $filename
make check file=$dir/$filename userincl="${*:2}"
rm $dir/$filename

