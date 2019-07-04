# nodejswslfix #

This makes `npm install` work on WSL, i.e. Ubuntu on Windows.

In WSL, calls to rename() on a directory fail if a submember of that directory is open.
This is often due to concurrent behavior.
In npm, it seems it can be from processing sub-dependencies at the same time as a dependency.

This library intercepts the rename() call, informs the user of what it thinks is going on, and tries again.
It works to fix the issue for me.

### Quick Installation
    ln -s $(pwd)/*.so* /usr/lib

### Usage
    LD_PRELOAD=interceptrename.so npm install

### Required dependencies (to build)

https://github.com/pmem/syscall_intercept

### Building
    touch interceptrename.c
    make
