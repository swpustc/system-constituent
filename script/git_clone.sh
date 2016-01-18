#!/bin/bash

git clone . "$2" --branch $1 >/dev/null 2>&1

GIT_DIR="$2/.git"
GIT_WORK_TREE="$2"

commit=$(git show master | awk 'NR==1,NR==2'| grep commit | awk 'NR==1' | awk '{print $2}')
echo commit: $commit
touch-git "$2" >/dev/null 2>&1
