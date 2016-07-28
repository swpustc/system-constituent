#!/bin/bash

in_path=$1
f_build="$in_path/../master/tmp/GIT_HEAD_MASTER.rebuild"


NEW_COMMIT=$(GIT_DIR="$in_path/.git" GIT_WORK_TREE="$in_path" git show -s --pretty=format:%h | awk 'NR==1' | awk '{printf $1}')

if [ -f "$in_path/include/GIT_HEAD_MASTER" ] ; then
  OLD_COMMIT=$(cat "$in_path/include/GIT_HEAD_MASTER" | awk 'NR==1' | awk '{printf $3}')
fi


if [ "$NEW_COMMIT"x != "$OLD_COMMIT"x ] ; then
  echo "Update new GIT_HEAD_MASTER ($NEW_COMMIT)..."
  echo "#define GIT_HEAD_MASTER $NEW_COMMIT">"$in_path/include/GIT_HEAD_MASTER"
fi

if [ "$NEW_COMMIT"x == "$OLD_COMMIT"x ] ; then
  if [ -f "$f_build" ] ; then
    rm "$f_build"
  fi
  exit 1
else
  touch "$f_build"
  exit 0
fi
