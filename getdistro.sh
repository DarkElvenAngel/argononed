#!/bin/sh

# find string case insensitive and return our distroname
# make sure to have specific entries listed first (see opensuse)
check_distro() {
    lower_case_string=`echo $1 | tr '[:upper:]' '[:lower:]'`
    case $lower_case_string in 
        *alma*) distro='almalinux';;
        *alpine*) distro='alpine';;
        *archarm*) distro='archarm';;
        *debian*) 
            RPI_KERNEL=$(dpkg --list | grep raspberrypi-kernel | cut -c 1-2)
            [ "x$RPI_KERNEL" = "xii" ] && distro="rpios" || distro="debian"
            ;;
        *fedora*)
            VARIANT_ID="`(awk -F"=" '$1=="VARIANT_ID"{print $2}' /etc/os-release | sed 's/\"//g')`"
            [ "x$VARIANT_ID" = "x" ] && distro="fedora" || distro="${lower_case_string}-${VARIANT_ID}"
            ;;
        *gentoo*) distro='gentoo';;
        *kali*) distro='kali';;
        *lakka*) distro='lakka';;
        *libreelec*) distro='libreelec';;
        *manjaro*) distro='manjaro';;
        *manjaro-arm*) distro='manjaro';;
        *opensuse-leap*) distro='opensuse-leap';;
        *opensuse-microos*) distro='opensuse-micoos';;
        *opensuse-tumbleweed*)='opensuse-tumbleweed';;
        *opensuse*)='opensuse';;
        *openwrt*) distro='openwrt';;
        *osmc*) distro='osmc';;
        *picore*) distro='piCore';;
        *pop*) distro='pop';;
        *raspian*) distro='raspbian';;
        *ubuntu*) distro='ubuntu';;
        *void*) distro='void';;
        *) distro='UNKNOWN';;
    esac
}

distro="UNKNOWN"
# special checks
while distro="UNKNOWN"
do
  [ -f '/etc/rpi-issue' ] && { distro='rpios'; break; }
  [ -f /usr/share/doc/tc/release.txt ] && { distro=piCore; break; }
  break
done
# check by command
[ "$distro" = "UNKNOWN" ] && [ -f /etc/os-release ] && check_distro "`awk -F"=" '$1=="ID"{print $2}' /etc/os-release | sed 's/\"//g'`"
[ "$distro" = "UNKNOWN" ] && [ `command -v lsb_release` ] && check_distro "`lsb_release -sid`" 
[ "$distro" = "UNKNOWN" ] && [ `command -v uname` ] && check_distro "`uname -sr`" 
[ "$distro" = "UNKNOWN" ] && [ `command -v hostnamectl` ] && check_distro "`hostnamectl`" 

echo "$distro"
[ "$distro" = "UNKNOWN" ] && exit 1 || exit 0