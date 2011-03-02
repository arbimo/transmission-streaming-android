#!/bin/sh

mkdir -p ./debian/usr/bin
cp ../../daemon/transmission-daemon ./debian/usr/bin
cp ../../daemon/transmission-remote ./debian/usr/bin
# vi control
mkdir -p debian/DEBIAN
cp control debian/DEBIAN
fakeroot dpkg-deb --build debian
mv debian.deb transmission-streaming_2.0.4-1_i386.deb
rm -rf debian
