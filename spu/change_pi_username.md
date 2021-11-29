# Change Pi username on Raspbian OS

Info from [https://forums.raspberrypi.com/viewtopic.php?f=63&t=227620#p1396338](https://forums.raspberrypi.com/viewtopic.php?f=63&t=227620#p1396338
)
OBS: this script must run before first boot. Mount the partition on a Linux machine and execute it.

```bash
cd ~
mkdir sources
cd ~/sources
```

```bash
cat <<- "EOF" | tee ~/sources/script.sh
#!/bin/sh

# remind the user what they are up to
echo "Script to change username 'pi' in a new Raspberry Pi image"
echo "This should be run within root directory of a fresh disk image"
echo "BEFORE it has been booted for first time"
echo ""

# $USER is set in shell so don't need this
#USER=$(/usr/bin/whoami)
# check if root
echo "running as $USER"
[ $USER != 'root' ] && echo "aborting because need to run as root" && exit 1
echo ""

# get the new user name
read -p "Enter username to replace 'pi': " NEWNAME
# bail out if blank
[ -z $NEWNAME ] && echo "aborting because no name provided" && exit 1
echo ""

# ask if they want to proceed
echo "Renaming 'pi' user to '$NEWNAME'"
# get a confirmation response,
read -p "proceed? " RESPONSE
# convert to lower case and just first character
RESPONSE=$(echo "$RESPONSE" | tr '[:upper:]' '[:lower:]' | cut -c 1)
# bail out unless that was 'y'
[ $RESPONSE != 'y' ] && exit 1

# passwd file has username as first field, and also update the home directory
echo "etc/passwd ..."
echo "============"
cp etc/passwd etc/passwd_pre-rename
/bin/sed -i -r -e "s/^pi:/$NEWNAME:/1" etc/passwd
# change home directory
# note using # as sed substitute character to avoid trouble with directory '/' chars
/bin/sed -i -r -e "s#:/home/pi:#:/home/$NEWNAME:#1" etc/passwd
/bin/grep -e ^$NEWNAME etc/passwd
echo ""

# shadow file has username as first field
echo "etc/shadow ..."
echo "============"
cp etc/shadow etc/shadow_pre-rename
/bin/sed -i -r -e "s/^pi:/$NEWNAME:/1" etc/shadow
/bin/grep -e ^$NEWNAME etc/shadow
echo ""

# group file has list of usernames as last field
echo "etc/group ..."
echo "============"
cp etc/group etc/group_pre-rename
# rename the group that is called pi
/bin/sed -i -r -e "s/^pi:/$NEWNAME:/1" etc/group
/bin/grep -e ^$NEWNAME etc/group
# now rename all the usernames within groups
# this is where only a single user is in the group
/bin/sed -i -r -e "s/:pi$/:$NEWNAME/1" etc/group
# pi is first / mid / last user in a multi-user group respectively 
/bin/sed -i -r -e "s/^(.*:.*:.*:)pi,(.*)$/\1$NEWNAME,\2/1" etc/group
/bin/sed -i -r -e "s/^(.*:.*:.*:.*),pi,(.*)$/\1,$NEWNAME,\2/1" etc/group
/bin/sed -i -r -e "s/^(.*:.*:.*:.*),pi$/\1,$NEWNAME/1" etc/group
/bin/grep -e ".*:.*:.*:.*$NEWNAME" etc/group
echo ""

# gshadow file is same structure as group
echo "etc/gshadow ..."
echo "============"
cp etc/gshadow etc/gshadow_pre-rename
# rename the group that is called pi
/bin/sed -i -r -e "s/^pi:/$NEWNAME:/1" etc/gshadow
/bin/grep -e ^$NEWNAME etc/gshadow
# now rename all the usernames within groups
# this is where only a single user is in the group
/bin/sed -i -r -e "s/:pi$/:$NEWNAME/1" etc/gshadow
# pi is first / mid / last user in a multi-user group respectively 
/bin/sed -i -r -e "s/^(.*:.*:.*:)pi,(.*)$/\1$NEWNAME,\2/1" etc/gshadow
/bin/sed -i -r -e "s/^(.*:.*:.*:.*),pi,(.*)$/\1,$NEWNAME,\2/1" etc/gshadow
/bin/sed -i -r -e "s/^(.*:.*:.*:.*),pi$/\1,$NEWNAME/1" etc/gshadow
/bin/grep -e ".*:.*:.*:.*$NEWNAME" etc/gshadow
echo ""

# sudowers file has username as first field, with whitespace as separator
echo "etc/sudoers ..."
echo "============"
cp etc/sudoers etc/sudoers_pre-rename
# rename pi entry
/bin/sed -i -r -e "s/^pi(\s)/$NEWNAME\1/1" etc/sudoers
/bin/grep -e ^$NEWNAME etc/sudoers
echo ""

# rename home directory
echo "rename home directory ..."
echo "============"
/bin/mv home/pi home/$NEWNAME
ls -l home
echo ""
EOF
```

```bash
sudo chmod +X ~/sources/script.sh
~/sources/script.sh
```
