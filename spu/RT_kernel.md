# Compile RT kernel

Info at [https://lemariva.com/blog/2018/04/raspberry-pi-rt-preempt-tutorial-for-kernel-4-14-y](https://lemariva.com/blog/2018/04/raspberry-pi-rt-preempt-tutorial-for-kernel-4-14-y)

```bash
echo -e "raspberry\nmappings\nmappings" | passwd
sudo apt update
sudo apt upgrade -y
sudo apt install -y git
```

I'm currently using _rpi-5.10.y_, so I've only downloaded that branch (~2.75 GB)

`git clone -b rpi-5.10.y https://github.com/raspberrypi/linux.git`

To patch the kernel with the Preempt-RT patch, we need the patch to match the kernel version. To look for the right patch execute `head Makefile -n 4` at the folder you just cloned.

I got the following info:

```bash
# SPDX-License-Identifier: GPL-2.0
VERSION = 5
PATCHLEVEL = 10
SUBLEVEL = 78
```

So we need rt patch for 5.10.78. At 

