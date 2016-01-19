#!/bin/bash

REMOTE=$(GIT_DIR="$1/.git" GIT_WORK_TREE="$1" git remote -v | grep origin | grep fetch | awk '{print $2}')
COMMIT=$(GIT_DIR="$1/.git" GIT_WORK_TREE="$1" git show | awk 'NR==1,NR==2'| grep commit | awk 'NR==1' | awk '{print $2}')

GIT_DIR="$2/.git"
GIT_WORK_TREE="$2"
cd $2

git remote set-url origin $REMOTE >/dev/null 2>&1
git fetch --prune
git checkout $COMMIT -f           >/dev/null 2>&1

commit=$(git show | awk 'NR==1,NR==2'| grep commit | awk 'NR==1' | awk '{print $2}')
echo commit: $commit
