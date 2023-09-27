# Fedora install

The file `argononed.conf` is copied to `/etc/argononed.conf` during install. It contains:  

```conf
# Uncomment to change default values
# fans = 10, 55, 100
# temps = 55, 60, 65
# hysteresis = 3

# SET_HWMON_NUM flag, use 0x0C in service mode or 0x0E in foreground mode
flags = 0x0C
```

## Troubleshooting

If you encounter errors in the reports form `argonone-cli --decode` like these

```text
...
Daemon Critical Errors: xxx <-- Keep increasing
```

Please check `/var/log/argononed.log`

You should see errors like this:

```log
[ CRITICAL ] i2c bus at /dev/i2c-1 inaccessible
```

### The solution

1. From the [Offical Fedora Documents](https://fedoraproject.org/wiki/Architectures/ARM/Raspberry_Pi/HATs#General_configuration):
   To make i2c work, we need to run the following commands:

    ```sh
    sudo su
    rm /boot/dtb
    echo "FirmwareDT=True" >> /etc/u-boot.conf
    ```

1. From the [Fedora Discussion Thread here](https://discussion.fedoraproject.org/t/74434/5), we need to add some lines to `/boot/efi/config.txt`.

    * Case 1:  
   Run command `i2cdetect -l`\
   If your output look like this:

   ```text
   i2c-1    i2c           bcm2835 (i2c@7e804000)              I2C adapter
   i2c-20    i2c           Broadcom STB :                      I2C adapter
   i2c-21    i2c           Broadcom STB :                      I2C adapter
   ```

   Add `dtparam=i2c_arm=on` to `/boot/efi/config.txt`

   * Case 2:  
   Run command `ls /sys/class/pwm/pwmchip0`\
   If your output look like this:

   ```text
   device  export  npwm  power  subsystem  uevent  unexport  
   ```

   Run command `lsmod | grep pwm`\
   If your output look like this:'

   ```text
   pwm_bcm2835            16384  0
   ```

   Add `dtoverlay=pwm,pin=18,func=2` to `/boot/efi/config.txt`
