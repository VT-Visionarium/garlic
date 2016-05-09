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
 cmake-doc\
 cmake\
 lynx\
 at\
 markdown\
 aptitude\
 talk\
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
 imagemagick-doc\
 imagemagick\
 lynx\
 markdown\
 libasound2-dev\
 libasound2-doc\
 libpulse-dev\
 alsa-tools\
 alsa-utils\
 libreoffice\
 vim-gtk\
 vim-doc\
 sox\
 libsox-fmt-all\
 xmlstarlet\
 consolekit\
 re2c\
 bison\
 bison-doc\
 nfs-common\
 libxt-dev\
 libqt4-dev\
 mysql-utilities\
 || exit $?

# this will remove wayland
# this is dangerous.  We need to reboot after this

#apt-get install xorg || exit 1

#apt-get install python-pip paraview python-astropy python-scipy python2.7-dev


apt-get -y install sshfs

apt-get -y install\
 libcr-dev\
 mpich2\
 mpich2-doc\
 mesa-utils\
 libglu1-mesa-dev\
 cmake-curses-gui\
 || exit $?

# This software updater is a pain, it runs 'apt-get update'
# and 'apt-get dist-upgrade'
apt-get -y purge update-manager || exit $?

