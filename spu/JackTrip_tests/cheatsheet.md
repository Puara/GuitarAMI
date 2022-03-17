# Jacktrip experiment conductor cheatsheet

## Check if both jacktrip boxes are connected to internet

`mosquitto_sub -h jacktrip001.local -t message`

## Check if jacktrip client and server have an audio connection

`systemctl --user status jacktrip_server.service`

The status should be *Received Connection from Peer!*. If not, reboot the client box.

Press `q` to go back to the terminal.

## Make Channel 1 stereo:

```bash
jack_connect jacktrip_client.local:send_2 system:capture_1
jack_connect jacktrip_client.local:receive_1 system:playback_2
```

## Latency test

`jack_delay -O jacktrip_client.local:send_2 -I jacktrip_client.local:receive_2 > latency.txt`

You can replace the file name (latency.txt) if needed.

## Copy the latency test files

Execute `startx` and use the *File Manager* (yellow folder icon) to copy the text files to a USB stick.

## Commom pitfalls

- Both client and server audio interfaces must have phantom power (48V) **ON** if using condenser microphones.
- Gain for channel 1 should be enough for tou to have a *green ring* around the knob when speaking.
- Adjust the *monitor* knob to listen confortably.
- Make sure the *Direct Monitor*  button is **OFF** to avoid listening to yourself when talking. 

## OPTIONAL - Change Jack audio interface settings

Run the `patchbox` command, go to **jack**, then **config**, then **USB**.

The usual configuration is:

- 44100
- 256
- 3

