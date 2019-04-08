= nodejswslfix =

In WSL (Ubuntu on Windows), calls to rename() on a directory fail if a submember of that directory is open.
This is often due to concurrent behavior.  This library catches the rename() call and waits for 1 second to try again if it fails.
This fixes the issue when it occurs in `npm install`.

Required dependencies:
	https://github.com/pmem/syscall_intercept

Building:
	make

Usage:
	LD_PRELOAD=interceptrename.so npm install

