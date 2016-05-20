#!/bin/bash

# on anvil ran:

if [ "$(id -u)" != "0" ] ; then
    echo "Run this $0 as root"
    exit 1
fi

set -x
apt-get --yes install apache2 apache2-doc php5 php-pear


# made users public_html/ homepages available via

cd /etc/apache2/mods-enabled/ || exit $?
ln -s ../mods-available/userdir.load . || exit $?
ln -s ../mods-available/userdir.conf . || exit $?

set +x
echo "Hit ENTER to vim edit /etc/apache2/mods-available/php5.conf"
echo "Comment out all the lines that control php_admin_value engine off"
echo -n " ==Ctrl-C to exit====<y> "
read a
echo

set -x
vim /etc/apache2/mods-available/php5.conf || exit $?
# and comment out php_admin_value engine off
#     in the Directory /home/*/public_html

service apache2 restart

