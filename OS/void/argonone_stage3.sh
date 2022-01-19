#!/bin/bash
# This script is written for VOID LINUX to run during stage 3
# and should be called by /etc/rc.shutdown 

REBOOT=$(stat -c "%a" /run/runit/reboot)
[[ $((0$REBOOT & 0100)) -ne 0 ]] && /usr/sbin/shutdown_argonone poweroff
exit 0