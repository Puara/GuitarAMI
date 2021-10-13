# JackTrip tests on SPU

Official information on building JackTrip on Rpi4 available at [https://help.jacktrip.org/hc/en-us/articles/1500009727561-Build-a-Raspberry-Pi-4B-Computer-with-JackTrip](https://help.jacktrip.org/hc/en-us/articles/1500009727561-Build-a-Raspberry-Pi-4B-Computer-with-JackTrip)

## Compiling and running JackTrip headless on the SPU

- Dependencies: `sudo apt install libjack-jackd2-dev librtaudio-dev qt5-default`
- cloning and building JackTrip:

    ```bash
    cd ~/sources
    git clone https://github.com/jacktrip/jacktrip.git
    cd ~/sources/jacktrip/src
    ./build
    export JACK_NO_AUDIO_RESERVATION=1
    ```

To use as a client: `./jacktrip -C [xxx.xx.xxx.xxx] -q16 -z -n2`

Adding a service to start JackTrip server:

```bash
cat <<- "EOF" | tee ~/.config/systemd/user/jacktrip.service
[Unit]
Description=Run JackTrip service
After=multi-user.target

[Service]
Type=idle
Restart=always
ExecStart=/home/patch/sources/jacktrip/builddir/jacktrip -s

[Install]
WantedBy=default.target
EOF
```

```bash
sudo chmod 644 ~/.config/systemd/user/jacktrip.service
systemctl --user daemon-reload
systemctl --user enable jacktrip.service
```
