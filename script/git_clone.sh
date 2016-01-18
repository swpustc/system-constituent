#!/bin/bash

git clone . "$1" >/dev/null 2>&1

GIT_DIR="$1/.git"
GIT_WORK_TREE="$1"

commit=$(git show master | awk 'NR==1,NR==2'| grep commit | awk 'NR==1' | awk '{print $2}')
echo commit: $commit
touch-git "$1" >/dev/null 2>&1
