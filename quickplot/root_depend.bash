#!/bin/bash

# you need to run this as root

if [ "$(id -u)" != 0 ] ; then
    echo "Run this as root"
    exit 1
fi


set -x

apt-get -y install\
 autoconf\
 autoconf-doc\
 libtool\
 libtool-doc\
 automake\
 libsndfile-dev\
 libreadline-dev\
 libgtk2.0-dev\
 libgtk2.0-doc\
 libgtk-3-dev\
 libgtk-3-doc\
 libasound-dev\
 imagemagick-doc\
 imagemagick\
 libasound2-doc\
 libpulse-dev\
 lynx\
 markdown

