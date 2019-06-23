#!/bin/bash

apt-get update
apt-get install build-essential libtool autoconf libconfig-dev
apt-get install linux-headers-`uname -r`
git clone https://github.com/jackmitch/libsoc.git
cd libsoc
autoreconf -i
./configure
make
make install
cd ..      
git clone https://github.com/libusbgx/libusbgx.git
cd libusbgx
autoreconf -i
./configure
make
make install
ldconfig
cd ..
./autogen.sh
./configure
make
make install
git clone https://gist.github.com/vaishnav98/c084ebd8bc0531e714baf2413770b5c3 dummy_hcd
cd dummy_hcd
wget https://raw.githubusercontent.com/beagleboard/linux/4.14/drivers/usb/gadget/udc/dummy_hcd.c
make 
cp dummy_hcd.ko /lib/modules/`uname -r`/kernel/drivers/usb/gadget/legacy/
depmod
cp startgbsim /usr/bin/startgbsim
cp systemd/gbsim.service /etc/systemd/system/
systemctl daemon-reload
systemctl enable gbsim
cd ..

