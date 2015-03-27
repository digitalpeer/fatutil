#!/bin/sh

set -e

./genfat.sh

rm -rf test.dat

mkdir test.dat
dd if=/dev/urandom of=test.dat/file1 bs=1 count=100
dd if=/dev/urandom of=test.dat/file2 bs=100 count=500
mkdir test.dat/dir1
dd if=/dev/urandom of=test.dat/dir1/file3 bs=3 count=1024

sha1sum test.dat/file1 test.dat/file2 test.dat/dir1/file3

./fatutil fatfs.img write test.dat/file1 /file1
./fatutil fatfs.img write test.dat/file2 /file2
./fatutil fatfs.img mkdir /dir1
./fatutil fatfs.img write test.dat/dir1/file3 /dir1/file3

./fatutil fatfs.img ls /
./fatutil fatfs.img ls /dir1

rm -f test.dat/file1 test.dat/file2 test.dat/dir1/file3

./fatutil fatfs.img read /file1 test.dat/file1
./fatutil fatfs.img read /file2 test.dat/file2
./fatutil fatfs.img read /dir1/file3 test.dat/dir1/file3

./fatutil fatfs.img unlink /file1
./fatutil fatfs.img unlink /file2
./fatutil fatfs.img unlink /dir1/file3
./fatutil fatfs.img rmdir /dir1

./fatutil fatfs.img ls /

sha1sum test.dat/file1 test.dat/file2 test.dat/dir1/file3
