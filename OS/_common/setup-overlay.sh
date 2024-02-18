#!/bin/sh -x
TEST_FOR_E=`echo -e 'E'`
[ "$TEST_FOR_E" = '-e E' ] && ECHO="echo" || ECHO="echo -e"

[ $# -eq 0 ] && FILE=/boot/config.txt || FILE=$1
[ -w "$FILE" ] || { $ECHO "  ERROR Cannot Write to ${FILE} unable to continue"; exit 1; }

$ECHO -n "  search ${FILE} for overlay ... "
grep -i '^dtoverlay=argonone' $FILE 1> /dev/null && { $ECHO "FOUND"; exit 0; } || $ECHO "NOT FOUND"

SYSMODEL=$( awk '{ print $0 }' /proc/device-tree/model | sed 's|Raspberry Pi||;s|Rev.*||;s|Model||;s|Zero|0|;s|Plus|+|;s|B| |;s|A| |;s| ||g' )
[ "x$SYSMODEL" = "x" ] && SECTION='ALL' || SECTION="pi${SYSMODEL}"

cp $FILE $FILE.backup
$ECHO -n "  insert overlay into ${FILE} ... "
if [ `grep -i "^\[${SECTION}\]" $FILE >/dev/null 2>&1` ]
then
    sed -i "/^\[${SECTION}\][^\n]*/I,\$!b;//{x;//p;g};//!H;\$!d;x;s//&\ndtoverlay=argonone/" $FILE && { $ECHO "DONE"; exit 0; } || { $ECHO "Failed"; exit 1; }
else
    $ECHO "[${SECTION}]\ndtoverlay=argonone" >> $FILE && { $ECHO "DONE"; exit 0; } || { $ECHO "Failed"; exit 1; }
fi
exit 0
