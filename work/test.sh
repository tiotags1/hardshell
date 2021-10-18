#!/usr/bin/hardshell

UID=1000
ROOT=/tmp/hello1

newid $UID

setroot $ROOT

ro /etc /usr /bin /sbin /lib /lib64 /opt
mount $ROOT/home/$USER /home/$USER
#procfs /proc
devfs /dev
#sysfs /sys
tmpfs /var /tmp /run
mkdir /run/user/$UID

cd /home/$USER

/bin/sh


