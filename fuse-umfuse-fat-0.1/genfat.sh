#!/bin/sh

#
# Example of creating an 100M fat filesystem image.
#

dd if=/dev/zero of=fatfs.img bs=1M count=100

mkfs.vfat fatfs.img

