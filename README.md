# nodejswslfix #

This makes `npm install` work on WSL, i.e. Ubuntu on Windows.

In WSL, calls to rename() on a directory fail if a submember of that directory is open.
This is often due to concurrent behavior.
In npm, it seems it can be from processing sub-dependencies at the same time as a dependency.

This library intercepts the rename() call and waits for 1 second to try again if it fails.
It works to fix the issue for me.

### Required dependencies

https://github.com/pmem/syscall_intercept

### Building
    make

### Usage
    LD_PRELOAD=interceptrename.so npm install

