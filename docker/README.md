# DOCKER !?

**IMPORTANT** This is meant to run on an ARM CPU.

## What this is for

If you need to build argononed for a 32bit system and you are on an arm64 system then this is what you need.  Having switch to arm64 myself there are issues with building for some targets.  I wanted an easy to use solution, so docker to the rescue!

## How to use this container

The `build.sh` script is what you need it takes care of everything! _(mostly)_

As of now we only build self installing package scripts that are used for a few platforms _eg. libreelec_

To build a self installing package script you switch to the docker folder and run

`./build.sh <TARGET_OS> [BRANCH]` 

* **TARGET_OS** - is the OS you want the package for _eg. libreelec_
* **BRANCH** - is optional by default **master** is built

The container is then built, run, the file is copied to the `docker/output/` folder, and finally the container is removed.  The image will be left behind.

## Help wanted

I know this is very basic and has limited functionality.  My goal was to keep things small so I used the Alpine Linux base image.  Where you can help, since I just learned docker this area could be improved.  I welcome your input.