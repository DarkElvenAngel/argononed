# Building ArgonOneD on piCore

piCore is the _raspberry_ port of [TinyCore Linux (Microcore)](http://tinycorelinux.net/). And this is kind of special. Two main glitches compared to a normal linux distro are:

* when the OS is running there's no /boot mounted or available anymore
* installation differs a lot due to
   * BusyBox init system
   * Persistence concept

# System setup

The instrauction has been created based on a rather bare piCore setup [Install piCore on Raspi SD card](https://www.irkode.de/raspi/picoreSD/)
Tested on a Pi 4B rev1.1 running piCore 13.1

## Install Toolchain

Tinycore provides a general build tool package containing everything you need, but git. Lets install both:

```bash
tce-load -wi compiletc sstrip
tce-load -wi git
```

## Install Device Tree Compiler (DTC)

DTC is not available as a package, so we will have to build it manually. Luckily it works OOTB without any problem.

```bash
cd ~
git clone https://git.kernel.org/pub/scm/utils/dtc/dtc.git
cd dtc
make
### lots of messages
./dtc -v
Version: DTC 1.6.1-gff5afb96
# copy to a folder contained in PATH, so configure can find it
cp dtc ~/.local/bin
```

## Configure ArgonOneD

While installing the service we need to have write access to `/boot` on the boot partition to install the overlay and add it to `config.txt`. We are on a running system so this is not available anymore.

By now I found no way to programatically find the boot partition used for the running instance. So you will have to know it.

For a SD card installation it should be /dev/mmcblk0p1. So we can simply mount it using `mount /dev/mmcblk0p1` as piCore provides default mount points for each partition and for our example this will be `/mnt/mmcblk0p1`.

```bash
sudo mount /dev/mmcblk0p1
ls /mnt/mmcblk0p1/config.txt
#/mnt/mmcblk0p1/config.txt
ls -d /mnt/mmcblk0p1/*/
#/mnt/mmcblk0p1/overlays/
```

Now we are able to pass this as `BOOTLOC` to configure.

Make also sure the distro is detected and the dependency check is successsful

```bash
cd ~
git clone https://gitlab.com/DarkElvenAngel/argononed.git
cd argononed
BOOTLOC=/mnt/mmcblk0p1 ./configure
#ARGON ONE DAEMON CONFIGURING ...
#Distro check [piCore] : OK
#SYSTEM CHECK
#gcc : OK
#dtc : OK
#make : OK
#I2C Bus check : NOT ENABLED
#CHECKING OPTIONAL SYSTEMS
#bash-autocomplete : NOT INSTALLED
#logrotate : NOT INSTALLED
#Dependency Check : Successful
```

the bash-autocomplete and logrotate settings are not important. we will not install it as a tiny core lacks both

## Build ArgonOneD

Thats an easy one. Just `make` it.

```bash
make
#### lots of messages
ls -la build
#----
#drwxr-sr-x    2 tc       staff         4096 Jan  2 20:05 ./
#drwxr-sr-x    6 tc       staff         4096 Jan  2 20:03 ../
#-rwxr-xr-x    1 tc       staff        14536 Jan  2 20:05 argonone-cli
#-rwxr-xr-x    1 tc       staff         5532 Jan  2 20:05 argonone-shutdown
#-rw-r--r--    1 tc       staff          997 Jan  2 20:05 argonone.dtbo
#-rwxr-xr-x    1 tc       staff        18320 Jan  2 20:05 argononed
#-rw-r--r--    1 tc       staff        17544 Jan  2 20:05 argononed.o
#-rw-r--r--    1 tc       staff         3008 Jan  2 20:05 event_timer.o
```

## Install ArononeD

The correct way would be to package the App as tcz archive and install. For now our customized _make install_ will do the job and install quite bare, respecting TinCore file system loayout and persistence.

`sudo mount /dev/mmcblk0p1`

`sudo make install`

```
Installing daemon
  install argononed ... Successful
  install argonone-shutdown ... Successful
Installing argonone client
  install argonone-cli ... Successful
  link to /usr/local/bin/argonone-cli ...Successful
  persist link in /opt/.filetool.lst ...Successful
Installing services
  add I2C modules to /opt/bootlocal.sh ... Successful
  add argononed to /opt/bootlocal.sh ... Successful
  add argonone-shutdown to /opt/shutdown.sh ... Successful
Starting Service Successful
Installing overlay
  install overlay to /mnt/sdb1/overlays ...Successful
Updating /mnt/sdb1/config.txt
  search /mnt/sdb1/config.txt for overlay ... NOT FOUND
  insert overlay into /mnt/sdb1/config.txt ... DONE
  activate I2C bus ... skipped - already there
Install Complete
```

Note: Its ok if the Service could not be started, as you will miss the overlay and config.txt entries at bootup.

Now all should be in place. **Before** rebooting we have to save installed files.

Use `filetool.sh -d` to check if all files are covered, or directly look at `/opt/.filetool.lst`

   * `opt/argononed`
   * `usr/local/bin/argonone-cli`

persist using `filetool.sh -b`

and `sudo reboot`

## Test

Quick tests are:

```bash
# should list the two needed modules
lsmod | grep i2c_
# ...
# i2c_bcm2708            16384  0
# i2c_dev                20480  0
# ...

# should print status and no errors
argonone-cli --decode

# Fan to full speed so you can hear it
./argonone-cli -m -f 100
# Reset to auto mode (if your pi is not too hot, the fan will stop)
./argonone-cli -a
```

## Uninstall

To remove the package just use the _uninstall_ target. 

This will not revert the `i2c_arm=on` entry in `config.txt`, this may still be in use by other software installed by other means.

`sudo make uninstall`

```
Uninstalling
  stop service ... Successful
  delete application folder /opt/argononed ... Successful
  delete /usr/local/bin/argonone-cli ... Successful
  remove overlay /mnt/mmcblk0p1/overlays/argonone.dtbo ... Successful
  Cleanup system files
    remove usr/local/bin/argonone-cli from /opt/.filetool.lst ... Successful
    delete /opt/argononed/argonone-shutdown from /opt/.filetool.lst ... Successful
    delete /sbin/modprobe -a i2c-dev i2c-bcm2708 from /opt/.filetool.lst ... Successful
    delete /opt/argononed/argononed from /opt/.filetool.lst ... Successful
    delete dtoverlay=argonone from /mnt/mmcblk0p1/config.txt ... Successful
  NOTE: we did not remove dtparam=i2c_arm=on.
        In case you don't need it anymore, just delete the entry
```
