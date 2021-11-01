# JackTrip tests on Raspberry OS

Using Rpi 4B + Raspbian OS + Pisound or USB audio Interface

Official information on building JackTrip on Rpi4 available at [https://help.jacktrip.org/hc/en-us/articles/1500009727561-Build-a-Raspberry-Pi-4B-Computer-with-JackTrip](https://help.jacktrip.org/hc/en-us/articles/1500009727561-Build-a-Raspberry-Pi-4B-Computer-with-JackTrip)

## Prepare SD card

- Download [Raspberry OS](https://www.raspberrypi.com/software/)
- Flash the image using [ApplePiBaker](https://www.tweaking4all.com/hardware/raspberry-pi/applepi-baker-v2/), [balenaEtcher](https://www.balena.io/etcher/), or diskutil (dd)
- Create an ssh file at the SSD boot folder: `touch ssh`
- Create a `wpa_supplicant.conf` file with the following contents (change SSID and password!) and boot the system (Rpi)

    ```bash
    cat <<EOF >wpa_supplicant.conf
        ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
        update_config=1
        country=CA

        network={
            ssid="your_real_wifi_ssid"
            scan_ssid=1
            psk="your_real_password"
            key_mgmt=WPA-PSK
        }
    EOF
    ```

## First config

- ssh to the Rpi: `ssh pi@ip_address` (The default user name is *pi* and its password is *raspberry*)
- Run `sudo raspi-config`
  - Update
  - Change password (mappings)
  - Change hostmane (SPU001)
  - Set WiFi Country (in system options, Wireless LAN)
  - Set locale (en_CA UTF-8 UTF-8)
  - Set timezone
  - Expand filesystem
- reboot
- Update the system:
  - `sudo apt update`
  - `sudo apt dist-upgrade`
- Install Vim as an optional editor: `sudo apt -y install vim`
- Create some folders: `mkdir ~/sources ~/Documents ~/.config/systemd/ ~/.config/systemd/user`

## Install Jack2

[http://jackaudio.org/faq/build_info.html](http://jackaudio.org/faq/build_info.html)

- Dependencies:
  
  `sudo apt-get install libsamplerate0-dev libsndfile1-dev libasound2-dev libavahi-client-dev libreadline-dev libfftw3-dev libudev-dev libncurses5-dev cmake git`

- compile and install jackd (no d-bus):

    ```bash
    cd ~/sources
    git clone git://github.com/jackaudio/jack2 --depth 1
    cd ~/sources/jack2
    ./waf configure --alsa --libdir=/usr/lib/arm-linux-gnueabihf/
    ./waf build
    sudo ./waf install
    sudo ldconfig
    sudo sh -c "echo @audio - memlock 256000 >> /etc/security/limits.conf"
    sudo sh -c "echo @audio - rtprio 75 >> /etc/security/limits.conf"
    echo /usr/local/bin/jackd -P50 -dalsa -p128 -n2 -r48000 > ~/.jackdrc
    ```

- set Jack to start at boot:

    ```bash
    cat <<- "EOF" | sudo tee /lib/systemd/system/jackaudio.service
    [Unit]
    Description=JACK Audio
    After=sound.target

    [Service]
    ExecStart=jackd -P50 -t2000 -d alsa -d hw:USB -p128 -n2 -r48000 -s &

    [Install]
    WantedBy=multi-user.target
    EOF

    ```

    ```bash
    sudo chmod 644 /lib/systemd/system/jackaudio.service
    sudo systemctl daemon-reload
    sudo systemctl enable jackaudio.service
    sudo systemctl start jackaudio.service
    ```

- Reboot

Some commands:

- Check available devices: `cat /proc/asound/cards`. If you have multiple devices available, can call them by name
- List informtion and connections on ports: `jack_lsp -c`
- Connect ports: `jack_connect [ -s | --server servername ] [-h | --help ] port1 port2` (The exit status is zero if successful, 1 otherwise)
- Disconnect ports: `jack_disconnect [ -s | --server servername ] [ -h | --help ] port1 port2`

## Compiling and running JackTrip headless on the SPU

- Dependencies: `sudo apt install libjack-jackd2-dev librtaudio-dev qt5-default`
- Extra package to test latency: `sudo apt install jack-delay`
- cloning and building JackTrip:

    ```bash
    cd ~/sources
    git clone https://github.com/jacktrip/jacktrip.git
    cd ~/sources/jacktrip/src
    ./build
    export JACK_NO_AUDIO_RESERVATION=1
    ```

## To manually use as a client

- with IP address: `./jacktrip -c [xxx.xx.xxx.xxx]`
- with name: `./jacktrip -c spuXXX.local`

## Adding a service to start JackTrip server

```bash
cat <<- "EOF" | sudo tee /lib/systemd/system/jacktrip_server.service
[Unit]
Description=Run JackTrip server
After=multi-user.target

[Service]
Type=idle
Restart=always
ExecStart=/home/pi/sources/jacktrip/builddir/jacktrip -s

[Install]
WantedBy=multi-user.target
EOF
```

```bash
sudo chmod 644 /lib/systemd/system/jacktrip_server.service
sudo systemctl daemon-reload
sudo systemctl enable jacktrip_server.service
sudo systemctl start jacktrip_server.service
```

## Adding a service to start JackTrip client (in this example, the server is spu003.local)

```bash
cat <<- "EOF" | sudo tee /lib/systemd/system/jacktrip_client.service
[Unit]
Description=Run JackTrip client
After=multi-user.target

[Service]
Type=idle
Restart=always
ExecStart=/home/pi/sources/jacktrip/builddir/jacktrip -c spu005.local

[Install]
WantedBy=multi-user.target
EOF
```

```bash
sudo chmod 644 /lib/systemd/system/jacktrip_client.service
sudo systemctl daemon-reload
sudo systemctl enable jacktrip_client.service
sudo systemctl start jacktrip_client.service
```

## Mapping using jack in CLI

- `jack_lsp`: lists jack available ports
- `jack_lsp -c`: lists jack available ports with connections
