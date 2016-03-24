#!/bin/bash

set -x

apt-get -y install\
 manpages-dev\
 manpages-posix-dev\
 octave\
 gnuplot\
 gnuplot-doc\
 doxygen\
 doxygen-doc\
 cscope\
 htop\
 strace\
 rsync\
 gdebi\
 csh\
 xmlstarlet\
 vim\
 tree\
 cmake\
 lynx\
 at\
 markdown\
 aptitude\
 talk\
 cmake-doc || exit $?

# this will remove wayland
# this is dangerous.  We need to reboot after this
#apt-get install libgl1-mesa-dev libgl1-mesa-glx xorg mesa-utils libglu1-mesa-dev || exit 1

apt-get -y install\
 libreoffice\
 vim-gtk\
 vim-doc\
 sox\
 libsox-fmt-all\
 python-pip\
 paraview\
 python-astropy\
 python-scipy\
 libcr-dev\
 mpich2-doc\
 mpich2\
 mesa-utils\
 cmake-curses-gui\
 python2.7-dev\
 python-astropy || exit $?

# optional libraries for a better VRUI experience
apt-get -y install\
    libudev-dev\ 
    libusb-1.0-0-dev\ 
    libpng-dev\ 
    libjpeg-dev\ 
    libtiff-dev\ 
    libasound-dev\
    libdc1394-22-dev\ 
    libspeex-dev\ 
    libogg-dev\ 
    libtheora-dev\ 
    libbluetooth-dev\ 
    libopenal-dev || exit $?

apt-get -y purge update-manager || exit $?


