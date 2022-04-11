# Builing Instructions - GuitarAMI Sound Processing Unit

- [Builing Instructions - GuitarAMI Sound Processing Unit](#builing-instructions---guitarami-sound-processing-unit)
  - [BOM](#bom)
  - [Prepare SD card](#prepare-sd-card)
  - [First configuration](#first-configuration)

## BOM

- [Raspberri Pi 4 model B](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/)
- [Pisound](https://blokas.io/pisound/) (embedded)
- external audio interface support
- [Custom PREEMPT-RT kernel](RT_kernel.md) (build on 5.10)
- [Raspberry Pi OS Lite](https://www.raspberrypi.com/software/operating-systems/)

## Prepare SD card

- Download the [Raspberry Pi OS Lite](https://www.raspberrypi.com/software/operating-systems/)
- Flash the image using [ApplePiBaker](https://www.tweaking4all.com/hardware/raspberry-pi/applepi-baker-v2/), [balenaEtcher](https://www.balena.io/etcher/), or diskutil (dd)
- enable ssh with `touch ssh` in the **boot** partition (SD card)

## First configuration

- ssh to the SPU: `ssh pi@ip_address` or `ssh pi@raspberrypi.local` (the default user name is *pi* and its password is *raspberry*)

```bash
sudo apt update -y &&
sudo apt upgrade -y &&
sudo raspi-config
```

Change the following:

- update
- Localization options -> WLAN Country
- Localization options -> Timezone
- Advanced Options -> Expand Filesystem
- System Options -> Hostname -> `SPUXXX`
- System Options -> password -> `mappings`
- System Options -> Boot / Auto Login -> Console
