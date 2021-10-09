#!/bin/bash

if [ -e /home/patch/Patches/guitarami.update ]; then
    sudo chmod +x /home/patch/Patches/guitarami.update
    /home/patch/Patches/guitarami.update
fi