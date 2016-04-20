#!/bin/bash

set -x

apt-get -y install\
 manpages-dev\
 manpages-posix-dev\
 octave\
 gnuplot\
 autoconf\
 automake\
 libtool\
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

#apt-get install xorg || exit 1

#apt-get install python-pip paraview python-astropy python-scipy python2.7-dev


apt-get -y install sshfs

apt-get -y install\
 libreoffice\
 vim-gtk\
 vim-doc\
 sox\
 libsox-fmt-all\
 libcr-dev\
 mpich2\
 mpich2-doc\
 mesa-utils\
 libglu1-mesa-dev\
 cmake-curses-gui\
 || exit $?

apt-get -y purge update-manager || exit $?

