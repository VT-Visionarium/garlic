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
 talk\
 aptitude\
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
 mpich2\
 mpich2-doc\
 mesa-utils\
 python-astropy || exit $?

apt-get -y purge update-manager || exit $?

