modprobe greybus
modprobe gb-es2
modprobe gb-gbphy
modprobe gb-i2c
modprobe gb-spi
modprobe gb-gpio
modprobe gb-uart
modprobe configfs
mount -t configfs none /sys/kernel/config
modprobe libcomposite
insmod dummy_hcd/dummy_hcd.ko
mkdir -p /tmp/gbsim/hotplug-module
gbsim