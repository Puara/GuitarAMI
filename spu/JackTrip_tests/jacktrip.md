# JackTrip tests on SPU

Official information on building JackTrip on Rpi4 available at [https://help.jacktrip.org/hc/en-us/articles/1500009727561-Build-a-Raspberry-Pi-4B-Computer-with-JackTrip](https://help.jacktrip.org/hc/en-us/articles/1500009727561-Build-a-Raspberry-Pi-4B-Computer-with-JackTrip)

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
cat <<- "EOF" | tee ~/.config/systemd/user/jacktrip_server.service
[Unit]
Description=Run JackTrip server
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
sudo chmod 644 ~/.config/systemd/user/jacktrip_server.service
systemctl --user daemon-reload
systemctl --user enable jacktrip_server.service
```

## Adding a service to start JackTrip client (in this example, the server is spu003.local)

```bash
cat <<- "EOF" | tee ~/.config/systemd/user/jacktrip_client.service
[Unit]
Description=Run JackTrip client
After=multi-user.target

[Service]
Type=idle
Restart=always
ExecStart=/home/patch/sources/jacktrip/builddir/jacktrip -c spu003.local

[Install]
WantedBy=default.target
EOF
```

```bash
sudo chmod 644 ~/.config/systemd/user/jacktrip_client.service
systemctl --user daemon-reload
systemctl --user enable jacktrip_client.service
```

## Mapping using jack in CLI

- `jack_lsp`: lists jack available ports
- `jack_lsp -c`: lists jack available ports with connections
