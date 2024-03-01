# Argon One Daemon

A replacement daemon for the Argon One Raspberry Pi cases, and the Argon Artik Fan Hat.

This is a Mirror of https://gitlab.com/DarkElvenAngel/argononed

Please note that this is not up to date and no issues are accepted on this mirror please direct all issues to the main project.

## What's new in 0.4.x

* More control: multiple ways to configure.
* Full rewrite of the base code.

## These features are moved to 0.5.x

* Support for Argon V3 and Pi 5
* Support for Argon IR REMOTE.
* More robust IPC support.
* New cli tool `argonctl`

## Why make this?

Simply put I didn't like the OEM software.  It works sure but it uses Python and needs to install a bunch of dependencies.  This makes it's foot print on your system much bigger than it needs to be.  My daemon runs with minimal requirements, and requires no extra libraries.

## OS Support

The current list of supported OS's are  

* Raspberry Pi OS 32bit or 64bit
* RetroPi
* Gentoo
* Manjaro-arm
* Arch Linux arm (ARMv7 installation ONLY) [aarch64/AUR](OS/archarm/README.md)
* Ubuntu
* OSMC
* TwisterOS
* DietPI
* Pop OS
* Kali Linux
* AlmaLinux Thanks to @ArclightMat
* Void Linux
* Lakka *\**
* LibreElec *\**
* [OpenWRT](OS/openwrt/README.md) **EXPERIMENTAL** *\**
* [Alpine Linux](OS/alpine/README.md) **SEE LINK**
* opensuse Thanks to @fridrich
* opensuse-microos **EXPERIMENTAL**
* [piCore](OS/piCore/README.md) Thank to @irkode
* [Vanilla Debian](OS/debian/README.md) see link.
* Fedora-IoT Thanks to @jwillikers for working through
* [Fedora-Server](OS/fedora-server/README.md) Thanks to @mjackdk and @ItzMiracleOwO **See Link for troubleshooting**
* [NixOS](OS/nixos/README.md) **SPECIAL** *(Please see link for this OS)* Thanks to @ykis-0-0 for all the hard work required for this one

If your OS isn't on this list it means that the installer isn't setup for your OS and it *may* or *may not* be able to install on your system. If your OS is based off one of these there is a strong chance it will *just* work.

*\** *Support for this OS is with the self extracting package system. SEE BELOW*

## How To Install

Firstly you need to have a build environment setup, that includes the following `gcc dtc git bash linux-headers make git`

*NOTE : The package names will be different depending on your OS so I've only given their binary names. Refer to your distribution for what you need to install.*  

I've tried to make the installer as simple as possible. After cloning this repo simply run ```./install``` You may need to reboot for full functionality.

## Configuration

There are more ways to configure the daemon than before this is useful for advanced users however general users only need worry about using the `config.txt`.

Using multiple configuration methods gives the greatest range of flexibility to this software.  The load order is important, the primary configuration comes from the overlay settings these are loaded first and have the lowest priority, next the configuration file is loaded, lastly the command line arguments are read they have the highest priority.  Values are overwritten from lowest to highest priority with one acceptation the flags value.  Flags are added together only the argument --force-flag-reset can set the flags to the default value this is set at compile time.

What happens if there is no configuration file? The daemon will check if the configuration file exists if it's not found it's noted in the log but will not throw an error. Even if a custom file is requested.

### Overlay Config

Configuration starts in the **config.txt** look for this line ```dtoverlay=argonone``` The parameters are simple.

* **fantemp[0-2]** - Sets the temperatures at which the fan will spin up
* **fanspeed[0-2]** - Sets the speed at which the fan will spin
* **hysteresis** - Sets the hysteresis

The default values are the same as the OEM at 55℃ the fan will start at 10%, at 60℃ the speed will increase to 55% and finally after 65℃ the fan will spin at 100%.  The default hysteresis is 3℃

Version 0.1.0 of the overlay has additional settings

* **argonconf** - Set the location of argononed.conf *default /etc/argononed.conf*
* **argon-i2c** - Set the i2c bus used for the controller *default 1*
* **argon-flag** - This is used to set build flags *default 0* ** See Advanced Build Options for values*

This overlay version also presets the GPIO 4 as and input with it's pull down resistor enabled.

#### Example config.txt

In this example the hysteresis will be set to 5 and the fan will start at 50℃

```text
dtoverlay=argonone,hysteresis=5
dtparam=fantemp0=50
```

### Configuration File

The default configuration is loaded from the `config.txt` via the *device-tree* how ever it's possible to use a configuration file instead.  The configuration file will override most settings from `config.txt`. The default configuration file can be placed at `/etc/argononed.conf` you can customize this location with `argonconf=FILENAME` in config.txt or with the `--conf=FILENAME` argument set to **argononed**.  The following are the properties and the accepted values.

* fans - set all fan speeds at once each value is comma separated.
* fan*X* - set specific fan speed where *X* is a number from 0 - 2.
* temps - set all temperature set points at once each value is comma separated.
* temp*X* - set specific temperature set point where *X* is a number from 0 - 2.
* hysteresis - set the hysteresis values 0 - 10 are valid
* loglevel - change the log level of the daemon see Logging Options for values.
* i2cbus - change the i2c bus address default is 1 and this value is rarely changed.
* flags - see Advanced Build Options for values. **IMPORTANT this value is in HEX format**

#### Example configuration file

```conf
# argononed configuration
fans = 50, 75, 100
temps = 50, 60, 65
hysteresis = 10

flags = 0x04
```

### Command Line Arguments

The daemon now accepts command line arguments.

```text
      --fan0=VALUE           Set Fan1 value
      --fan1=VALUE           Set Fan2 value
      --fan2=VALUE           Set Fan3 value
      --fans=VALUE           Set Fan values
      --hysteresis=VALUE     Set Hysteresis
      --temp0=VALUE          Set Temperature1 value
      --temp1=VALUE          Set Temperature2 value
      --temp2=VALUE          Set Temperature3 value
      --temps=VALUE          Set Temperature values
      --conf=FILENAME        load config
      --forceflag=VALUE      Force flags to VALUE
  -F, --forground            Run in Forground
  -l, --loglevel=VALUE       Set Log level
  -c, --colour               Run in Forground with colour
      --dumpconf             Dump build config
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version
```

These are subject to change before release.

#### Example comand line

`# argononed -cl6 --conf=/tmp/argononed.conf` This will start the in foreground with colour output, set the log level to 6 *DEBUG* and set the configuration file to **/tmp/argononed.conf**

### Final word on configuration

If you are an advanced user and need to easily set multiple configurations it is recommended not to use build flags to disable/force default features that can be set with flags.  As these features will be unavailable to switch on without recompile.

An example of this would be the **DISABLE_POWERBUTTON** flag if you build with this option enabled it will no longer be possible to use the power button.

If you make a typo in the configuration file you will get warnings in log.  This will also set the **EF_CONF** flag.  Likewise a mistake on the command line arguments will set the *EF_ARG** flag.

## Upgrading to the latest version

In order to upgrade to the latest version the current method is to pull the updates from gitlab and execute the following command

```text
./install
```

## Logging Options

The default build will generate a very detailed logs if you want less logging then add  
```make LOGLEVEL=[0-6]```  
The log levels go in this order: NONE, FATAL, CRITICAL, ERROR, WARNING, INFO, DEBUG. A value of 0 disables logging. It is possible to turn logging back on or change the level using the `-l#` command line argument.

## Advanced Build Options

 Advanced Build options are used with `configure` or `package.sh`. Each of these options has a flag counterpart that can be set in config.txt, the configuration file, or a command line argument.

 *Note that once these are set there is no way to unset them unless you recompile.*

 **DISABLE_POWERBUTTON** *or* **FLAG 0x01** if you don't have `/dev/gpiochip0` or you don't want to use the power button then use this flag.  Remember that the Force shutdown >= 5 second long press will still work.

 **RUN_IN_FOREGROUND** *or* **FLAG 0x02** if you need the daemon to always run in the foreground this flag will skip the forking to the background and cause the daemon to log to the console.

 **USE_SYSFS_TEMP** *or* **FLAG 0x04** If your system doesn't have `/dev/vcio` you'll need to use the sysfs temperature sensor set.

## Fan Modes

The Daemon has 4 modes of operation but will always starts in Automatic mode.  Change modes using the GUI Applet is available or cli tool.

### Cool Down Mode

In cool down mode the fan has a set temperature you want to reach before switching back to automatic control.  This is all set as follows   ```argonone-cli --cooldown <TEMP> [--fan <SPEED>]```  
***NOTE***: *The speed is optional and the default is 10% it's also import to note that if the temperature continues to climb the schedules set for the fan are ignored.*  

### Manual Mode  

As the name implies your in control over the fan the schedules are ignored.  To access this as follows ```argonone-cli --manual [--fan <SPEED>]```  
***NOTE***: *The fan speed is optional and if not set the fans speed is left alone.*

### Auto Mode

This is the default mode the daemon always starts in this mode and will follow the schedules in the setting.  If you want to change to automatic you do so as follows ```argonone-cli --auto```

### Off Mode

Yes an off switch, maybe you want to do something and you need to be sure the fan doesn't turn on and spoil it.  You can turn off the fan as follows ```argonone-cli --off```
***NOTE***: *When the fan is off nothing but turning to a different mode will turn it back on*

## `argonone-cli` tool

The `argonone-cli` command line tool lets you change setting on the fly. It communicates with shared memory of the daemon, so the daemon must be running for this tool to be of use.

## Setting schedules

Want to adjust the when the fan comes on, maybe it's not staying on long enough you can change all set points in the schedules from the command line **without** rebooting.  the values are fan[0-2] temp[0-2] and hysteresis.  It's important when changing these values that you remember that the daemon will reject bad values and/or change them to something else.  It's also important to commit the changes you make otherwise they won't do anything.  The value rules are simple each stage must to greater than the one before it and there are minimum and max values.  
For temperature the minimum value is 30° the maximum is currently undefined.  
For the fan the minimum speed is 10% and the maximum is 100%.  
For Hysteresis the minimum is 0° and the maximum is 10°  

You can set your values like in this example.  

```text
argonone-cli --fan0 25 --temp0 50 --hysteresis 10 --commit
```  

The following method is not supported.

```text
argonone-cli --fan0 25
argonone-cli --temp0 50
argonone-cli --hysteresis 10
argonone-cli --commit
```

The changes **MUST** in one shot and have `--commit` them for them to take effect.

## New error flags

New error flags are being added to share memory and will soon be available in a memory decode. The goal of these flags is to make determining the source of an error more obvious without the need to dive through logs that may be disabled depending on your build options.

```text
>> DECODEING MEMORY <<
Fan Status OFF Speed 0%
System Temperature 45°
Hysteresis set to 10°
Fan Speeds set to 10% 55% 100%
Fan Temps set to 55° 60° 65°
Fan Mode [ AUTO ] 
Fan Speed Override 0% 
Target Temperature 0° 
Daemon Status : Waiting for request
Maximum Temperature : 45°
Minimum Temperature : 44°
Daemon Warnings : 0
Daemon Errors : 0
Daemon Critical Errors : 3
```

This is the current output of `argonone-cli --decode` you can see something is wrong but what it is we have to look into the logs.

```text
>> DECODEING MEMORY <<
Fan Status OFF Speed 0%
System Temperature 45°
Hysteresis set to 10°
Fan Speeds set to 10% 55% 100%
Fan Temps set to 55° 60° 65°
Fan Mode [ AUTO ] 
Fan Speed Override 0% 
Target Temperature 0° 
Daemon Status : Waiting for request
Maximum Temperature : 45°
Minimum Temperature : 44°
Daemon Warnings : 0
Daemon Errors : 0
Daemon Critical Errors : 3
Error Flag(s) set
    i2c bus
```

With the flags you can see the error is with the i2c bus.

### Docker solution for packager

I have a docker solution for building packages read about it [here](docker/README.md)

## Argon Artik Hat

If you have an Argon Artik Hat and you see these error message:  

`Failed to load HAT overlay`

`dterror: not a valid FDT - err - 9`  

Check out my [firmware fix](firmware/README.md).
