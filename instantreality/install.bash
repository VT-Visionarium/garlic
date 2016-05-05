#!/bin/bash

# TODO: this script if kind of useless but it's has info in it.

# NOTE: We installed this without using encap and that's okay.
#
# If your looking for a better install.bash example, that uses source
# code, see file in /usr/local/src/quickplot

# We used gdebi to install instantreality from a deb file.  gdebi will
# also install missing dependencies using apt-get

#apt-get install gdebi

# Looks like you need to do more work than running gdebi to install
# multiple versions of instantreality.

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path


############################# STUFF TO CONFIGURE ########################

# see ftp://ftp.igd.fraunhofer.de/outgoing/irbuild/Ubuntu-x86_64/
# for other versions
VERSION=14.04-x64-2.6.0.31047

DEBSRC=$scriptdir/InstantReality-Ubuntu-${VERSION}.deb

#########################################################################

# We ran the following to inspect this package:

# tmp="$(mktemp -d)"; cd $tmp && ar x $DEBSRC && tar -xvJf data.tar.xz && ls -R .
# rm $tmp


echo -e "Please read the notes in this script $0\n"

echo -e "Run:\n\n  sudo gdebi \"$DEBSRC\"\n\nto install it"

echo -e "\nOr something like that.  Also consider installing a license via something like:\n"

echo -e "  sudo cp my_license.xml /opt/instantReality/bin/license.xml\n"
