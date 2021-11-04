# JackTrip tests on SPU

Official information on building JackTrip on Rpi4 available at [https://help.jacktrip.org/hc/en-us/articles/1500009727561-Build-a-Raspberry-Pi-4B-Computer-with-JackTrip](https://help.jacktrip.org/hc/en-us/articles/1500009727561-Build-a-Raspberry-Pi-4B-Computer-with-JackTrip)

## Compiling and running JackTrip headless on the SPU

- Dependencies: `sudo apt install libjack-jackd2-dev librtaudio-dev qt5-default`
- Extra package to test latency: `sudo apt install -y jack-delay`
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

OBS: client name is the name of the other machine

```bash
cat <<- "EOF" | tee ~/.config/systemd/user/jacktrip_server.service
[Unit]
Description=Run JackTrip server
After=multi-user.target

[Service]
Type=idle
Restart=always
ExecStart=/home/patch/sources/jacktrip/builddir/jacktrip -s --clientname spu005.local

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
ExecStart=/home/patch/sources/jacktrip/builddir/jacktrip -c spu003.local --clientname spu005.local

[Install]
WantedBy=default.target
EOF
```

```bash
sudo chmod 644 ~/.config/systemd/user/jacktrip_client.service
systemctl --user daemon-reload
systemctl --user enable jacktrip_client.service
```

## Install aj-snapshot

[http://aj-snapshot.sourceforge.net/](http://aj-snapshot.sourceforge.net/)

Check the last version on the website

```bash
sudo apt install -y libmxml-dev &&\
cd ~/sources &&\
wget http://downloads.sourceforge.net/project/aj-snapshot/aj-snapshot-0.9.9.tar.bz2 &&\
tar -xvjf aj-snapshot-0.9.9.tar.bz2 &&\
cd aj-snapshot-0.9.9 &&\
./configure &&\
make &&\
sudo make install
```

- To create a snapshot: `aj-snapshot -f ~/Documents/default.connections`
- To remove all Jack connections: `aj-snapshot -xj`
- To save connections: `sudo aj-snapshot -f ~/Documents/default.connections`
- To restore connections: `sudo aj-snapshot -r ~/Documents/default.connections`

Set the Jack connections to load at boot:

```bash
cat <<- "EOF" | sudo tee /lib/systemd/system/ajsnapshot.service
[Unit]
Description=AJ-Snapshot
After=sound.target jackaudio.service

[Service]
Type=oneshot
ExecStart=/usr/local/bin/aj-snapshot -r ~/Documents/default.connections

[Install]
WantedBy=multi-user.target
EOF
```

```bash
sudo systemctl daemon-reload &&\
sudo systemctl enable ajsnapshot.service
```

## Mapping using jack in CLI

- Check available devices: `cat /proc/asound/cards`. If you have multiple devices available, can call them by name
- lists jack available ports: `jack_lsp`
- List informtion and connections on ports: `jack_lsp -c` or `jack_lsp -A`
- Connect ports: `jack_connect [ -s | --server servername ] [-h | --help ] port1 port2` (The exit status is zero if successful, 1 otherwise)
- Disconnect ports: `jack_disconnect [ -s | --server servername ] [ -h | --help ] port1 port2`

## Latency tests

Make sure JackTrip is running.

- Connect the necessary audio cable to create a loopback on the audio interface (audii OUT -> audio IN)
- Remove all Jack connections of both machines: `aj-snapshot -xj`
- For the loopback (same interface test): `jack_delay -I system:capture_2 -O system:playback_2`
- For JackTrip (jack_delay running on the server):
  - on the server:
    - `jack_connect system:playback_2 spu005.local:receive_2`
    - `jack_connect system:capture_2 spu005.local:send_2`
  - on the client:
    - `jack_connect system:playback_2 spu003:receive_2`
    - `jack_connect system:capture_2 spu003:send_2`
- run the test: `jack_delay -O spu005.local:send_2 -I spu005.local:receive_2`
