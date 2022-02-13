#!/bin/sh
[ $# -eq 0 ] && FILE=/boot/config.txt || FILE=$1
[ -w "$FILE" ] || { echo "  ERROR Cannot Write to ${FILE} unable to continue"; exit 1; }
SYSMODEL=$( awk '{ print $0 }' /proc/device-tree/model | sed 's|Raspberry Pi||;s|Rev.*||;s|Model||;s|Zero|0|;s|Plus|+|;s|B| |;s|A| |;s| ||g' )

echo -n "  search ${FILE} for overlay ... "
grep -i '^dtoverlay=argonone' $FILE 1> /dev/null && { echo "FOUND"; exit 0; } || echo "NOT FOUND"
cp $FILE $FILE.backup
echo -n "  insert overlay into ${FILE} ... "
if [[ `grep -i "^\[pi${SYSMODEL}\]" $FILE >/dev/null 2>&1` ]]
then
    echo -e "[pi${SYSMODEL}]\ndtoverlay=argonone" >> $FILE && { echo "DONE"; exit 0; } || { echo "Failed"; exit 1; }
else
    sed -i "/^\[pi${SYSMODEL}\][^\n]*/I,\$!b;//{x;//p;g};//!H;\$!d;x;s//&\ndtoverlay=argonone/" $FILE && { echo "DONE"; exit 0; } || { echo "Failed"; exit 1; }
fi
exit 0
