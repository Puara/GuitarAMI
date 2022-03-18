# Compile RT kernel

- [Compile RT kernel](#compile-rt-kernel)
  - [Information sources](#information-sources)
  - [Instructions for compiling a generic Kernel (x86) in a Linux machine](#instructions-for-compiling-a-generic-kernel-x86-in-a-linux-machine)
  - [Cross-compiling Raspberry Pi Kernel on a Docker Container](#cross-compiling-raspberry-pi-kernel-on-a-docker-container)
  - [seila](#seila)

## Information sources

Official Linux Kernel Archives: [https://www.kernel.org/](https://www.kernel.org/)

Raspberry Pi Linux Kernels: [https://github.com/raspberrypi/linux](https://github.com/raspberrypi/linux)

Generic instructions to build Linux kernels: [https://www.linux.com/topic/desktop/how-compile-linux-kernel-0/](https://www.linux.com/topic/desktop/how-compile-linux-kernel-0/)

## Instructions for compiling a generic Kernel (x86) in a Linux machine

These instructions will compile version 5.15.28. Change the version and file names accordingly. You will need at least 12GB of free space (for this kernel version) on your local drive to get through the kernel compilation process. So make sure you have enough space.

Download the kernel source code: `wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.15.28.tar.xz`

Install dependencies: `sudo apt install -y git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison`

Extract the Kernel code and navigate to the created folder: `tar -xf linux-5.15.28.tar.xz && cd linux-5.15.28`

Before configuring the kernel, we need to create the config file. One easy way is to copy the existing one from the host system. On the kernel source-code folder, run: `cp /boot/config-$(uname -r) .config`. Note that `uname -r` returns the current kernel version.

To edit the Kernel settings, run `make menuconfig`.

To build: `make`, then install modules with `make modules_install`, and finally `sudo make install`.

## Cross-compiling Raspberry Pi Kernel on a Docker Container

Install Docker (info on [https://docs.docker.com/engine/install/ubuntu/](https://docs.docker.com/engine/install/ubuntu/)): 

```bash
sudo apt update -y && sudo apt upgrade -y && 
sudo apt install -y ca-certificates curl gnupg lsb-release &&
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg &&
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null &&
sudo apt -y update &&
sudo apt -y install docker-ce docker-ce-cli containerd.io
```

Check if Docker is running correctly with `sudo docker run hello-world`

Info about commands: `sudo docker --help`

https://github.com/jac4e/linux/wiki/Raspberry-Pi-Linux-Cross-compilation-Environment

https://www.raspberrypi.com/documentation/computers/linux_kernel.html

## seila

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

