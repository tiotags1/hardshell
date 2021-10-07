#!/usr/bin/hardshell

rw /etc /usr /bin /sbin /lib /lib64 /opt
procfs /proc
devfs /dev
tmpfs /var /tmp /run
mkdir /home/nobody
mkdir /run/user/1000
shell
