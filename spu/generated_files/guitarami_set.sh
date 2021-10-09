#!/bin/sh

# Use PiSound button to change the GuitarAMI sets (modep pedalboard + default SC patch + default PD patch)

sendosc 127.0.0.1 20000 /lcd s "clear" i 1 i 1
sendosc 127.0.0.1 20000 /lcd s "           GuitarAMI" i 1 i 1
sendosc 127.0.0.1 20000 /lcd s "Loading preset $1   " i 2 i 1

# replace spaces for underscores in file names

# for file in /home/patch/Patches/$1*/*' '*
# do
#  mv -- "$file" "${file// /_}"
# done

# set default SC file
if [ -e /home/patch/Patches/$1*/main.scd ]; then
    cp -f /home/patch/Patches/$1*/main.scd /home/patch/Patches/default.scd
elif [ -e /home/patch/Patches/default.scd ]; then
    rm /home/patch/Patches/default.scd
fi

# set default PD file
if [ -e /home/patch/Patches/$1*/main.pd ]; then
    cp -f /home/patch/Patches/$1*/main.pd /home/patch/Patches/default.pd
elif [ -e /home/patch/Patches/default.pd ]; then
    rm /home/patch/Patches/default.pd
fi

# set current MODEP pedalboard
if [ -e /home/patch/Patches/$1*/*.pedalboard ]; then
    pedalname=$(basename `find /home/patch/Patches/$1*/ -type d -name "*.pedalboard" -print -quit` .pedalboard)
    sudo cp -f -R /home/patch/Patches/$1*/$pedalname.pedalboard /var/modep/pedalboards/
    sudo chown -R modep:modep /var/modep/pedalboards/$pedalname.pedalboard
fi

# set permissions and restart services
sudo systemctl restart modep-mod-ui
# sendosc 127.0.0.1 20000 /lcd s "Loading             " i 2 i 1
# sendosc 127.0.0.1 20000 /lcd s $pedalname i 3 i 1

sleep 5

systemctl --user restart supercollider.service
systemctl --user restart puredata.service
python3 /usr/modep/scripts/modep-ctrl.py load-board "/var/modep/pedalboards/$pedalname.pedalboard"