#!/bin/bash

REMOTE=$(GIT_DIR="$1/.git" GIT_WORK_TREE="$1" git remote -v | grep origin | grep fetch | awk '{print $2}')

GIT_DIR="$2/.git"
GIT_WORK_TREE="$2"

git remote set-url origin $REMOTE >/dev/null 2>&1
git pull

commit=$(git show master | awk 'NR==1,NR==2'| grep commit | awk 'NR==1' | awk '{print $2}')
echo commit: $commit
touch-git "$2" >/dev/null 2>&1
