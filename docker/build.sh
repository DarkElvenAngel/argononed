#!/bin/bash 
echo -e "\e[H\e[J" 
echo -e "\e[37;1m    ___                                                __\e[0m"
echo -e "\e[37;1m   /   |  _________ _____  ____  ____  ____  ___  ____/ /\e[0m"
echo -e "\e[37;1m  / /| | / ___/ __ \`/ __ \/ __ \/ __ \/ __ \/ _ \/ __  / \e[0m"
echo -e "\e[37;1m / ___ |/ /  / /_/ / /_/ / / / / /_/ / / / /  __/ /_/ /  \e[0m"
echo -e "\e[37;1m/_/  |_/_/   \__, /\____/_/ /_/\____/_/ /_/\___/\__,_/   \e[0m"
echo -e "\e[37;1m            /____/                                       \e[0m"
echo -e "\e[37;1m                                                   BUILD \e[0m"
echo "_________________________________________________________"
[[ -n $1 ]] && TARGET_DISTRO=$1 || { echo -e "\e[37;1mERROR:\e[0m Target OS not set! USAGE $0 <TARGET OS> [GIT BRANCH]"; exit 1; }
[[ -n $2 ]] && GIT_BRANCH=$2
echo "BUILD PACKAGE FOR [ $TARGET_DISTRO ]"
docker build -t argone-compile . 2>/dev/null
docker run --name extract -e TARGET_DISTRO=$TARGET_DISTRO -e GIT_BRANCH=$GIT_BRANCH argone-compile 2>/dev/null
docker cp extract:/output/ ./ 2>/dev/null
docker rm extract &>/dev/null