#!/bin/bash

project=$1
outpath=$2
in_file=$3
pdbfile=$4


data_bin_path=$outpath/packages/$project/data/bin/
data_pdb_path=$outpath/packages/$project/data/pdb/

if [ -f "$in_file" ] ; then
  cp -p "$in_file" "$data_bin_path"
else
  echo Error: File: "$in_file" not exist.
  exit 1
fi

if [ -f "$pdbfile" ] ; then
  cp -p "$pdbfile" "$data_pdb_path"
else
  echo Error: File: "$in_file" not exist.
  exit 1
fi
