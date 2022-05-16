
* fixed not exiting due to infinite loop when loading commands from file
* better error message when execve fails
* mount run folder helpers
* added CAP\_MODE\_NOPRIV
* added a firefox profile

commit 28839149ae3a11693c27d85ecef36ceac9c0beeb
* finally entered chroot/sandbox area
* root is mounted as read only
* share/unshare command to set what namespaces to unshare
* newid command to set exit uid/gid
* setroot command to set sandbox location
* sysfs command to mount sysfs
* mount command to mount arbitrary folders
* cd command that sets current working dir after sandbox

commit 4848a96c8a28440e17c57c38924aba2174a361a2
* finally added ability to execute arbritrary processes
* pass command line arguments to subprocesses
* pass environment variables to subprocesses
* added quoted strings so you can add spaces inside strings (doesn't parse escape characters yet)

commit 17c38c09cc54400d6067edef0945c9791403c5f0
* added ability to use variables like in shell scripts
* set proper naming convention for internal functions
* optimized read buffer reallocation

commit db15d0613d6f6b760c31c6396371308702f4c679
* added cmake build system
* added comments
* added read from stdin or from file
* added a few basic operators: ro (readonly), rw (read-write), mkdir, devfs, procfs, tmpfs
* added a basic subprocess reaper
* added a recursive directory creation


