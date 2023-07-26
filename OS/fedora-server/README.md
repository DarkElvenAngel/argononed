The file `argononed.conf` is copied to `/etc/argononed.conf` during install. It contains:  
```
# Uncomment to change default values
# fans = 10, 55, 100
# temps = 55, 60, 65
# hysteresis = 3

# SET_HWMON_NUM flag, use 0x0C in service mode or 0x0E in foreground mode
flags = 0x0C
```
