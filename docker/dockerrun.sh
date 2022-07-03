#!/bin/bash
# set TARGET_DISTRO=$1
[[ -n $GIT_BRANCH ]] && { git checkout remotes/origin/$GIT_BRANCH &> /dev/null || exit 1; }
./configure
echo "CFLAGS+=-largp -static" >> makefile.conf
unset TARGET_DISTRO
./package.sh && cp build/*.sh /output
