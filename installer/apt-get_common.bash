#!/bin/bash

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
 rsync || exit 1

apt-get -y purge update-manager || exit 1
 
