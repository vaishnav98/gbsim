#!/bin/bash

board=$(sed "s/ /_/g;s/\o0//g" /proc/device-tree/model)
apt-get update
apt-get install build-essential libtool autoconf libconfig-dev
apt-get install linux-headers-`uname -r`
git clone https://github.com/jackmitch/libsoc.git
cd libsoc
autoreconf -i
./configure --disable-debug --enable-python --with-board-configs
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
wget https://raw.githubusercontent.com/vaishnav98/linux/4.14/drivers/usb/gadget/udc/dummy_hcd.c
make 
cp dummy_hcd.ko /lib/modules/`uname -r`/kernel/drivers/usb/gadget/legacy/
depmod
cd ..
cp startgbsim /usr/bin/startgbsim
cp systemd/gbsim@.service /etc/systemd/system/
systemctl daemon-reload
if [ "x${board}" = "xTI_AM335x_PocketBeagle" ] ; then
    systemctl enable gbsim@1
    systemctl enable gbsim@2
    systemctl start gbsim@1
    systemctl start gbsim@2    
else
    systemctl enable gbsim@1
    systemctl enable gbsim@2
    systemctl enable gbsim@3
    systemctl enable gbsim@4
    systemctl start gbsim@1
    systemctl start gbsim@2
    systemctl start gbsim@3
    systemctl start gbsim@4
fi
cd ..

