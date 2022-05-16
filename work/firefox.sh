#!/usr/bin/hardshell

UID=1000
ROOT=/tmp/firefox
HOME=/home/$USER/

newid $UID

setroot $ROOT

procfs /proc
devfs /dev
#sysfs /sys

ro /etc /bin /sbin /usr /lib /lib64
rw /tmp
# /var /run
mount $ROOT/home/$USER /home/$USER

ro /opt/firefox
mount_run dbus
mount_user_run dconf keyring

rw $HOME/Downloads
rw $HOME/.cache/mozilla $HOME/.mozilla
#rw $HOME/.pki $HOME/.local/share/pki

rw $HOME/.dbus $HOME/.config/dconf
rw $HOME/.config/gtk-2.0 $HOME/.config/gtk-3.0 $HOME/.config/gtk-4.0
rw /var/lib/dbus /var/cache/fontconfig /var/tmp /var/lock
#rw /var/lib/aspell /var/lib/ca-certificates /var/lib/menu-xdg /var/lib/uim

cd $HOME

#/bin/sh
/usr/bin/firefox-bin


