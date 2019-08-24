<!-- This file uses Github Flavored Markdown (GFM) format. -->

# Greybus Simulator (gbsim)

A tool which simulates an AP Bridge, SVC, and an arbitrary set
of Ara modules plugged into Greybus.

Provided under BSD license. See *LICENSE* for details.

## Quick Start

The install script installs the required dependencies and  sets up the Greybus Simulator as a startup service. The greybus kernel driver is required for proper working of GBSIM and is available on BeagleBone Linux Kernel Versions greater than 4.14.108-ti-r106, if you have an older version, please update the kernel. The details of image used for testing are:
```
debian@beaglebone:~$ uname -a
Linux beaglebone 4.14.108-ti-r106 #1 SMP PREEMPT Fri May 24 22:12:34 UTC 2019 armv7l GNU/Linux
debian@beaglebone:~$ cat /etc/dogtag
BeagleBoard.org Debian Image 2018-10-07
```

To update the kernel :
```
cd /opt/scripts/tools/
git pull
sudo ./update_kernel.sh
sudo reboot
```

For building and installing Greybus Simulator and Required Dependencies run the following script:
```
sudo sh installgbsim.sh
```
The script fetches and installs the simulator and required dependencies, make sure that the Beaglebone is connected to the internet while running the script.

## Run

Load up the Greybus Simulator corresponding to all the available Mikrobus Ports and the required modules manually, just run:

```
sudo startgbsim
```
by default after installation via the install script ,the gbsim will be set up a startup service whose status can be verified using these commands

```
 systemctl status gbsim
```

### Running GBSIM for a Single Mikrobus Port

For starting for single Mikrobus port, the parameters corresponding to the Port (SPI/I2C bus details) should be passed to gbsim:

```
gbsim -g GBSIMID -h HOTPLUGDIR -c CSNO -s SPIBUS -i I2CADAPTER
```

* `GBSIMID` : Unique Integer ID for GBSIM
* `HOTPLUGDIR` : GBSIM Hotplug Modules Directory
* `CSNO` : Mikrobus Port SPI Chip Select No.
* `SPIBUS` : Mikrobus Port SPI Bus No.
* `I2C ADAPTER` : Mikrobus Port I2C Adapter Number

The Mikrobus port details on different Boards are as shown below:

```
The pocketBeagle has two Mikrobus ports corresponding to :
  mikrobus port1 : SPI 1 CS 0  I2C 1
  mikrobus port2 : SPI 2 CS 1  I2C 2

on other beaglebone boards as two Mikrobus ports corresponding to :
  mikrobus port1 : SPI 1 CS 0  I2C 2
  mikrobus port2 : SPI 1 CS 1  I2C 2
  mikrobus port3 : SPI 0 CS 0  I2C 2
  mikrobus port4 : SPI 1 CS 2  I2C 2

```
### Using the simulator

More details on how to use Greybus Simulator with Mikroelektronika Clickboards is available here : [GBSIM Wiki](https://github.com/vaishnav98/gbsim/wiki)

This version of the simulator is modified to Support MikroElektronika Click Boards and the support for Click Boards can be tested by downloading and setting up the manifesto tool and the insclick/rmclick utilities.Manifest blob files can be created using the Manifesto tool
found at https://github.com/vaishnav98/manifesto. The click boards can be easily loaded/unloaded using the insclick/rmclick utilities
