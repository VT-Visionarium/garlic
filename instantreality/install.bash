#!/bin/bash

# NOTE: We installed this without using enap and that's okay.
#
# If your looking for a better install.bash example, that uses source
# code, see file in /usr/local/src/quickplot

# We used gdebi to install instantreality from a deb file.  gdebi will
# also install missing dependencies using apt-get

#apt-get install gdebi

# Looks like you need to do more work than running gdebi to install
# multiple versions of instantreality.



############################# STUFF TO CONFIGURE ########################

VERSION=14.04-x64-2.6.0.31047

DEBSRC=/usr/local/src/instantreality/\
InstantReality-Ubuntu-${VERSION}.deb

#########################################################################

# We ran the following to inspect this package:

# tmp="$(mktemp -d)"; cd $tmp && ar x $DEBSRC && tar -xvJf data.tar.xz && ls -R .
# rm $tmp

echo -e "Please read the notes in this script $0\n"

echo -e "Run:\n\n  sudo gdebi \"$DEBSRC\"\n\nto install it"

