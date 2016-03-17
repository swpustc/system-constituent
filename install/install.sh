#!/bin/bash

project=$1
in_path=$2
outpath=$3
verfile=$4
props_f=$5


data_path=$outpath/packages/$project/data
meta_path=$outpath/packages/$project/meta

if [ ! -d "$outpath" ] ; then
  mkdir   "$outpath"
fi
if [ ! -d "$outpath/packages" ] ; then
  mkdir   "$outpath/packages"
fi
if [ ! -d "$outpath/packages/$project" ] ; then
  mkdir   "$outpath/packages/$project"
else
  rm -fr  "$outpath/packages/$project"
  mkdir   "$outpath/packages/$project"
fi
mkdir "$data_path"
mkdir "$data_path/bin"
mkdir "$data_path/pdb"
mkdir "$meta_path"


ver_maj=$(cat "$verfile" | grep 'VERSION_MAJOR ' | awk 'NR==1' | awk '{printf $(NF)}')
ver_min=$(cat "$verfile" | grep 'VERSION_MINOR ' | awk 'NR==1' | awk '{printf $(NF)}')
ver_pt1=$(cat "$verfile" | grep 'VERSION_POINT ' | awk 'NR==1' | awk '{printf $(NF)}')
ver_pt2=$(cat "$verfile" | grep 'VERSION_POINT2' | awk 'NR==1' | awk '{printf $(NF)}')
ver_tmp=$(GIT_DIR="$in_path/.git" GIT_WORK_TREE="$in_path" git show -s --pretty=format:%ct | awk 'NR==1' | sed 's/</\&lt;/g' | sed 's/>/\&gt;/g' | sed 's#/#\\/#g')
if [ -f "$props_f" ] ; then
  ver_who=$(cat "$props_f" | grep 'QtBuildVersion' | awk 'NR==1' | awk -F\> '{printf $2}' | awk -F\< '{printf $1}' | sed 's/</\&lt;/g' | sed 's/>/\&gt;/g' | sed 's#/#\\/#g')
  if [ "$ver_who"x != ""x ] ; then
    ver=$ver_maj.$ver_min.$ver_pt1.$ver_pt2-$ver_tmp-$ver_who
  else
    ver=$ver_maj.$ver_min.$ver_pt1.$ver_pt2-$ver_tmp
  fi
else
  ver=$ver_maj.$ver_min.$ver_pt1.$ver_pt2-$ver_tmp
fi

date=$(GIT_DIR="$in_path/.git" GIT_WORK_TREE="$in_path" git show -s --pretty=format:%ci | awk 'NR==1' | awk '{printf $1}' | sed 's/</\&lt;/g' | sed 's/>/\&gt;/g' | sed 's#/#\\/#g')
commit=$(GIT_DIR="$in_path/.git" GIT_WORK_TREE="$in_path" git show -s --pretty=format:%s | awk 'NR==1' | sed 's/</\&lt;/g' | sed 's/>/\&gt;/g' | sed 's#/#\\/#g')

cat "$in_path/install/package.xml"   | \
  sed "s/\$(DisplayName)/$project/g" | \
  sed "s/\$(Version)/$ver/g"         | \
  sed "s/\$(ReleaseDate)/$date/g"    | \
  sed "s/\$(Commit)/$commit/g"       | \
  tee "$meta_path/package.xml"
