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
 vim-gtk\
 cscope\
 cmake\
 cmake-doc || exit 1

apt-get -y install\
 htop\
 tree\
 libreoffice\
 strace\
 csh\
 vim\
 vim-gtk\
 vim-doc\
 sox\
 libsox-fmt-all\
 rsync\
 gdebi\
 xmlstarlet || exit 1

apt-get -y purge update-manager || exit 1

# this will remove wayland
# this is dangerous.  We need to reboot after this
apt-get install libgl1-mesa-dev libgl1-mesa-glx xorg mesa-utils || exit 1


