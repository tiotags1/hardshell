
hardshell
=========

hardshell is yet another program that allows you to sandbox arbritrary linux applications. It's main goal is to provide a 'bash' like syntax for setting up containers.

Doesn't use any external dependencies besides libc and a linux kernel.

Doesn't require root/SUID.

Inspired by firejail (for the easy configuration options), bubblewrap (for not having configuration options), mkbox (for the ok function and basic sandbox setup).

pretty config file, check it out!

build & run
-----------

`mkdir -p build && cd build && cmake .. && make && cd ../work && ../build/hardshell test.sh`


