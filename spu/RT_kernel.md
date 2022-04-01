# Compile RT kernel

- [Compile RT kernel](#compile-rt-kernel)
  - [Information sources](#information-sources)
  - [Instructions for compiling a generic Kernel (x86) in a Linux machine](#instructions-for-compiling-a-generic-kernel-x86-in-a-linux-machine)
  - [Cross-compiling Raspberry Pi Kernel on a Docker Container](#cross-compiling-raspberry-pi-kernel-on-a-docker-container)
    - [Preparing the container](#preparing-the-container)
    - [Start compilation](#start-compilation)
    - [Deploying the new Kernel](#deploying-the-new-kernel)
    - [Configuring the Kernel](#configuring-the-kernel)
    - [Patching the Kernel](#patching-the-kernel)
  - [Info for next steps](#info-for-next-steps)

## Information sources

Official Linux Kernel Archives: [https://www.kernel.org/](https://www.kernel.org/)

Raspberry Pi Linux Kernels: [https://github.com/raspberrypi/linux](https://github.com/raspberrypi/linux)

Generic instructions to build Linux kernels: [https://www.linux.com/topic/desktop/how-compile-linux-kernel-0/](https://www.linux.com/topic/desktop/how-compile-linux-kernel-0/)

Tutorial about using Docker to cross-compile Rpi kernels in Ubuntu-based systems: [https://github.com/jac4e/linux/wiki/Raspberry-Pi-Linux-Cross-compilation-Environment](https://github.com/jac4e/linux/wiki/Raspberry-Pi-Linux-Cross-compilation-Environment)

Official instructions to build Rpi Linux kernels: [https://www.raspberrypi.com/documentation/computers/linux_kernel.html](https://www.raspberrypi.com/documentation/computers/linux_kernel.html)

Shrinking images: [http://www.aoakley.com/articles/2015-10-09-resizing-sd-images.php](http://www.aoakley.com/articles/2015-10-09-resizing-sd-images.php)

## Instructions for compiling a generic Kernel (x86) in a Linux machine

These instructions will compile version 5.15.28. Change the version and file names accordingly. You will need at least 12GB of free space (for this kernel version) on your local drive to get through the kernel compilation process. So make sure you have enough space.

Download the kernel source code: `wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.15.28.tar.xz`

Install dependencies: `sudo apt install -y git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison`

Extract the Kernel code and navigate to the created folder: `tar -xf linux-5.15.28.tar.xz && cd linux-5.15.28`

Before configuring the kernel, we need to create the config file. One easy way is to copy the existing one from the host system. On the kernel source-code folder, run: `cp /boot/config-$(uname -r) .config`. Note that `uname -r` returns the current kernel version.

To edit the Kernel settings, run `make menuconfig`.

To build: `make`, then install modules with `make modules_install`, and finally `sudo make install`.

## Cross-compiling Raspberry Pi Kernel on a Docker Container

### Preparing the container

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

Create a folder and navigate to it (e.g., `mkdir ~/sources/rpi_kernel_build && cd ~/sources/rpi_kernel_build`) and the *Dockerfile* file (change the manifesto to the proper Debian version):

```bash
cat <<- "EOF" | tee Dockerfile
FROM debian:bullseye

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get clean && apt-get update && \
    apt-get install -y \
    time \
    git bc sshfs bison flex libssl-dev make kmod libc6-dev libncurses5-dev \
    crossbuild-essential-armhf \
    crossbuild-essential-arm64

RUN mkdir /build
WORKDIR /build

CMD tail -f /dev/null
EOF
```

To build the image: `sudo docker build -t rpi-cross-compile .`

Check if the image is built correctly: `sudo docker images`

To run an instance of the Docker image (container) in Interactive Mode: `sudo docker run --device /dev/fuse --cap-add SYS_ADMIN --name rpi-cross-compile -it rpi-cross-compile bash`

You can exit with `exit` and the container will stop, but will not be removed.
If you want to jump back into it, you can run `docker start rpi-cross-compile`.

You can also attach and detach the container with `docker attach rpi-cross-compile` and **CTRL-p** **CTRL-q**. Note that this sequence will not work when on VSC. If you need to detach while using VSC it is recommended to run in an external terminal window.

### Start compilation

If detached of the container, use `docker attach rpi-cross-compile`.

Clone the Rpi linux repo (choose the proper branch): `git clone --depth=1 -b rpi-5.15.y https://github.com/raspberrypi/linux`

For compilation with the default parameters:

```bash
cd linux &&
KERNEL=kernel8 &&
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- bcm2711_defconfig
```

Compile (64bits) with *8* cores using `make -j8 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image modules dtbs LOCALVERSION=CUSTOM_VERSION_NUMBER`. DO NOT FORGET TO SET THE LOCALVERSION!

OBS: If you want to check compilation time: `/usr/bin/time --format="Compilation time: %E" make -j8 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image modules dtbs`.

OBS: If planning to compile multiple times, you can use `nproc` to check available cores and test with the quantity until you get an optimized compilation speed. Iterate until you find your best fit. Start from the nproc value, try upwards first and downwards only if the upwards attempts show immediate degradation. Check discussion [here](https://unix.stackexchange.com/questions/208568/how-to-determine-the-maximum-number-to-pass-to-make-j-option).

### Deploying the new Kernel

- [Install Directly onto the SD Card](https://www.raspberrypi.com/documentation/computers/linux_kernel.html#install-directly-onto-the-sd-card)

Copy the compiled kernel from the Docker container to a folder in the host computer: `sudo docker cp rpi-cross-compile:/build/linux ~/sources/rpi_kernel_build/linux`. Modify the destination folder if needed.

Use `lsblk` before and after plugging in your SD card to identify it. The smaller partition is usually the FAT system and the bigger the EXT4 system. E.g., **mmcblk0p1** and **mmcblk0p2**.

Navigate into the folder ontaining the kernel (e.g., `~/sources/rpi_kernel_build/linux`) and mount them:

```bash
mkdir mnt &&
mkdir mnt/fat32 &&
mkdir mnt/ext4 &&
sudo mount /dev/mmcblk0p1 mnt/fat32 &&
sudo mount /dev/mmcblk0p2 mnt/ext4
```

Install the kernel modules onto the SD card: `sudo env PATH=$PATH make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=mnt/ext4 modules_install`

Copy the kernel and Device Tree blobs onto the SD card USING A DIFFERENT KERNEL NAME to keep both standard and custom kernels:

```bash
sudo cp arch/arm64/boot/Image mnt/fat32/KERNEL-FILE-NAME.img &&
sudo cp arch/arm64/boot/dts/broadcom/*.dtb mnt/fat32/ &&
sudo cp arch/arm64/boot/dts/overlays/*.dtb* mnt/fat32/overlays/ &&
sudo cp arch/arm64/boot/dts/overlays/README mnt/fat32/overlays/ &&
sudo umount mnt/fat32 &&
sudo umount mnt/ext4
```

Boot the SD-card into the Pi.

To choose the loaded kernel automatically edit the `/boot/config.txt` file.

kernel=**KERNEL-FILE-NAME**.img

### Configuring the Kernel

From the [Raspberry Pi Documentation](https://www.raspberrypi.com/documentation/computers/linux_kernel.html#configuring-the-kernel):

Configuration is most commonly done through the make `menuconfig` interface. Alternatively, you can modify your `.config` file manually.

Using menuconfig: `make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig`

### Patching the Kernel

From the [Raspberry Pi Documentation](https://www.raspberrypi.com/documentation/computers/linux_kernel.html#patching-the-kernel)

## Info for next steps

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

So we need rt patch for 5.10.78. At ...
