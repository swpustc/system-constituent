#!/bin/bash

project=$1
in_file=$2
outpath=$3


data_bin_path=$outpath/packages/$project/data/bin/

if [ -f "$in_file" ] ; then
  cp -p "$in_file" "$data_bin_path"
else
  echo Error: File: "$in_file" not exist.
  exit 1
fi
