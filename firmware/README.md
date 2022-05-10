# Argon Artik Firmware

I have compiled a new firmware for the Argon Artik Hat.  The firmware is simply a fix for an error I encountered with Raspberry Pi OS Bullseye.

`Failed to load HAT overlay`

`dterror: not a valid FDT - err - 9`

These errors messages would appear after each boot although visual distracting and annoying they are harmless and now easy to fix.

## Disclaimer

This procedure is done at YOUR OWN RISK!!  
This software comes with ABSOLUTELY no warranty  

## How to update your hat

First you have to grab the software you can either clone the entire repo form raspberrypi's github or just grab the script.

The Link for the repo is [eepromutils](https://github.com/raspberrypi/hats/tree/master/eepromutils) or you can just grab the [eepflash.sh](https://raw.githubusercontent.com/raspberrypi/hats/master/eepromutils/eepflash.sh) script.

Once you have the script or repo downloaded find `eepflash.sh` This script is what we want to use.  (If you downloaded the repo the script is in `hats/eepromutils`) Now copy `artik_eeprom.eep` to the the same location as the script.  

**Now this is important.  BACKUP The current firmware, BACK IT UP, if anything goes wrong you can use the BACKUP to fix it, so make a BACKUP!**

Run the following command with root privileges *(sudo or as the root user)*  
`eepflash.sh -r -f=BACKUP.eep -t=24c32`  
Now if this works you should have `BACKUP.eep` make a copy, email it to yourself just don't loose that BACKUP.  

Now you can write the new firmware to do that, run the following command with root privileges *(sudo or as the root user)*  
`eepflash.sh -w -f=artik_eeprom.eep -t=24c32`

DONE!  

Now the test is to reboot and see if you get the message again.
