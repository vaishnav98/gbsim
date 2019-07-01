<!-- This file uses Github Flavored Markdown (GFM) format. -->

# Greybus Simulator (gbsim)

A tool which simulates an AP Bridge, SVC, and an arbitrary set
of Ara modules plugged into Greybus.

Provided under BSD license. See *LICENSE* for details.

## Quick Start

The install script installs the required dependencies and  sets up the Greybus Simulator as a startup service,the greybus driver is required for proper working of the simulator and is available on Linux beaglebone 4.14.108-ti-r106 kernel and later versions, if you have an older version, please update the kernel, the details of image used for testing are:
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

Load up the Greybus Simulator and the required modules manually, just run:

```
sudo startgbsim
```
by default after installation via the install script ,the gbsim will be set up a startup service whose status can be verified using these commands

```
 systemctl status gbsim
```

### Using the simulator
This version of the simulator is modified to Support MikroElektronika Click Boards and the support for Click Boards can be tested by downloading and setting up the manifesto tool and the insclick/rmclick utilities.Manifest blob files can be created using the Manifesto tool
found at https://github.com/vaishnav98/manifesto. The click boards can be easily loaded/unloaded using the insclick/rmclick utilities
