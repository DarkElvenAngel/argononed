# ARCH AUR Package

I'm not the maintainer of these package and have no direct way to contact who is, I'm adding a patch for [argonone-c-git](https://aur.archlinux.org/packages/argonone-c-git).

This will fix kernel issues where the user doesn't have access to the `/dev/vcio` and the build must use the `USE_SYSFS_TEMP` build flag. It's my hope these changes find there way into the AUR and I can help with any questions to improve farther.

I'm no expert on Arch Linux, and they most likely wouldn't like that I'm doing things this way. 

## How to use this AUR Package

To put it simply clone this repo and switch to the `OS/archarm/AUR` folder

From here you can run `makepkg` or you can add `LOGLEVEL=n makepkg` to adjust the loglevel see logging-options in the main README

I'm not going into detail how to use `makepkg` to install the package there are resources all over for that.

## Important Note

I'm not the original author of this package and am not a regular user of Arch Linux, as such I have no wish to take over as maintainer. However it seems fitting to maintain my own copy here as of this writing the package is _Orphaned_

## Bugs and Issues

Since I'm not maintaining on AUR if you need my help open an issue.  If you can help me open a merge request.

I hope this helps all my Arch Linux users.