# Vanilla Debian 

## Configuration Requirements

Debian needs a configuration file to function correctly the vanilla kernel doesn't support overlays so you cannot use the typical `config.txt` method.  The default location is `/etc/argononed.conf`

```conf
# argononed debian configuration
i2cbus = 3
flags = 0x05
```

These setting work in our testing.
