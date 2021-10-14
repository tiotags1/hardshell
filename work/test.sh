#!/usr/bin/hardshell

UID=1000

rw /etc /usr /bin /sbin /lib /lib64 /opt
procfs /proc
devfs /dev
tmpfs /var /tmp /run
mkdir /home/$USER
mkdir /run/user/$UID

/bin/sh


