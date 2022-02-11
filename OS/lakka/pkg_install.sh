F_INSTALL()
{
    #DATA_START=$((`grep -an "^DATA_CONTENT$" $0 | cut -d: -f1` + 1))
    echo -n "INFO:  Verify package list ... "
    tail -n+${DATA_START} $0 | tar tzv OS/lakka/pkg_list &>/dev/null && echo "FOUND!" || { echo "MISSING!"; exit 1; } 
    F_EXTRACT
    echo -n "INFO:  Verify contents ..."
    while read line; do
    # reading each line
    [[ -f $line ]] || { echo -e "ERR\nERROR:  ${line} File Not Found!"; exit 1; }
    done < OS/lakka/pkg_list
    echo "OK"
    echo "INFO:  Installing"
    [[ -d /storage/sbin ]] || mkdir /storage/sbin
    [[ -d /storage/.work ]] || mkdir /storage/.work
    [[ -d /storage/.systemd ]] || mkdir /storage/.systemd
    mount -t overlay overlay -o lowerdir=/lib/systemd,upperdir=/storage/.systemd,workdir=/storage/.work /lib/systemd
    install build/argononed /storage/sbin/argononed 2>/dev/null || { echo "ERROR:  Cannot install argononed"; exit 1;}
    install -m 0755 build/argonone-cli /storage/sbin/argonone-cli 2>/dev/null || { echo "ERROR:  Cannot install argonone-cli"; exit 1;}
    install -D build/argonone-shutdown /lib/systemd/system-shutdown/argonone-shutdown 2>/dev/null || { echo "ERROR:  Cannot install argonone-shutdown"; exit 1;}
    [[ -f /storage/.profile ]] || cp /etc/profile /storage/.profile
    grep /storage/sbin/ /storage/.profile &>/dev/null || echo "export PATH=/storage/sbin/:$PATH" >> /storage/.profile
    # Change path form /usr/sbin to /storage/sbin
    sed -i "s/usr/storage/g" OS/_common/argononed.service
    install OS/_common/argononed.service /storage/.config/system.d/argononed.service || { echo "ERROR:  Cannot install argononed.service"; exit 1;}
    install OS/lakka/argonone-shutdown.service /storage/.config/system.d/argonone-shutdown.service || { echo "ERROR:  Cannot install argononed-shutdown.service"; exit 1;}
    systemctl daemon-reload
    systemctl enable argononed &>/dev/null
    systemctl enable argonone-shutdown &>/dev/null
    mount -o remount,rw /flash
    install build/argonone.dtbo /flash/overlays
    sh OS/_common/setup-overlay.sh /flash/config.txt
    mount -o remount,ro /flash
    [[ -r /dev/i2c-1 ]] ||  echo "reboot required" && systemctl start argononed
}